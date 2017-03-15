#include "ImageRecognition.h"
#include <string>
#include <FreeImage.h>
#include <boost/filesystem.hpp>

ImageRecognition::ImageRecognition()
{
}

ImageRecognition & ImageRecognition::CreateNeuralNetwork(int32_t input, int32_t hidden, int32_t output, const std::string& data)
{
	m_OutputSize = output;
	m_InputSize = input;

	m_Data.set_data_file_name(data);
	m_Data.load_data();

	Variables* variables_pointer = m_Data.get_variables_pointer();
	for (int32_t i = 0; i < output; i++)
	{
		variables_pointer->set_use(input + i, Variables::Target);
	}

	m_NeuralNetwork.set(input,hidden,output);

	m_NeuralNetwork.construct_scaling_layer();
	ScalingLayer* scaling_layer_pointer = m_NeuralNetwork.get_scaling_layer_pointer();
	scaling_layer_pointer->set_scaling_method(ScalingLayer::NoScaling);

	m_NeuralNetwork.construct_probabilistic_layer();
	ProbabilisticLayer* probabilistic_layer_pointer = m_NeuralNetwork.get_probabilistic_layer_pointer();
	probabilistic_layer_pointer->set_probabilistic_method(ProbabilisticLayer::Probability);

	return *this;
}

ImageRecognition & ImageRecognition::LoadNeuralNetworkState(std::string dir)
{
	m_NeuralNetwork.load(dir + "ns.xml");
	m_TrainingStrategy.load(dir + "ts.xml");
	m_LossIndex.load(dir + "li.xml");
	return *this;
}

ImageRecognition & ImageRecognition::SaveNeuralNetworkState(std::string dir)
{
	m_NeuralNetwork.save(dir + "ns.xml");
	m_TrainingStrategy.save(dir + "ts.xml");
	m_LossIndex.save(dir + "li.xml");
	return *this;
}

ImageRecognition & ImageRecognition::SetData(std::string path)
{
	m_Data.set_data_file_name(path);
	m_Data.load_data();
	return *this;
}

ImageRecognition & ImageRecognition::RandomizeSamples()
{
	Instances* instP = m_Data.get_instances_pointer();
	instP->split_random_indices();
	return *this;
}

int64_t ImageRecognition::Train()
{
	m_LossIndex.set_neural_network_pointer(&m_NeuralNetwork);
	m_LossIndex.set_data_set_pointer(&m_Data);
	m_TrainingStrategy.set_loss_index_pointer(&m_LossIndex);

	auto start = std::chrono::system_clock().now();
	m_TrainingStrategy.perform_training();
	auto end = std::chrono::system_clock().now();
	int64_t m_TimeSEC = std::chrono::duration_cast<std::chrono::seconds>((end - start)).count();
	
	return m_TimeSEC;
}

int32_t ImageRecognition::CheckExampleFromFile(std::string path)
{
	const size_t imgW = static_cast<size_t>(sqrt(m_NeuralNetwork.get_inputs_number()));
	const size_t imgH = static_cast<size_t>(sqrt(m_NeuralNetwork.get_inputs_number()));

	FIBITMAP* bitmap = FreeImage_Load(FreeImage_GetFileType(path.c_str(), 0), path.c_str());
	if (!bitmap) return -1;

	bitmap = FreeImage_ConvertTo24Bits(bitmap);
	bitmap = FreeImage_Rescale(bitmap, imgW, imgH);

	Vector<double> input(m_NeuralNetwork.get_inputs_number());
	
	for (int32_t y = 0; y < imgH; y++)
	{
		for (int32_t x = 0; x < imgW; x++)
		{
			RGBQUAD pixel;
			FreeImage_GetPixelColor(bitmap, x, y, &pixel);
			int32_t value = (pixel.rgbBlue + pixel.rgbGreen + pixel.rgbRed) / 3;
			if (value < 250)
				value = 1;
			else
				value = 0;

			int index = (y*imgH + x);
			input[index] = value;
		}
	}
	FreeImage_Unload(bitmap);

	Vector< double > out = m_NeuralNetwork.calculate_outputs(input);

	uint32_t result = 0;
	for (int32_t i = 1; i < out.size(); i++)
	{
		if (out[i] > out[result])
			result = i;
	}
	return result;

}

int32_t ImageRecognition::CheckExampleFromVector(const std::vector<bool>& vector)
{
	Vector<double> input(m_NeuralNetwork.get_inputs_number());
	for (int32_t i = 0; i < input.size(); i++)
	{
		input[i] = vector[(i % 20) +  20* (19 - static_cast<int32_t>(i/20))];
	}
	Vector< double > out = m_NeuralNetwork.calculate_outputs(input);

	uint32_t result = 0;
	for (int32_t i = 1; i < out.size(); i++)
	{
		if (out[i] > out[result])
			result = i;
	}
	return result;
}