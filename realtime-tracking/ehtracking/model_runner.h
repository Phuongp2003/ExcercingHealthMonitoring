#ifndef MODEL_RUNNER_H
#define MODEL_RUNNER_H

#include "config.h"
#include "model_esp8266.h"

// TFLite globals
tflite::ErrorReporter *error_reporter = nullptr;
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

// Memory for TFLite arena
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// Flag to indicate if model is ready
bool modelInitialized = false;

// Constants for normalization to match training
// These values are extracted from the training data
#define IR_MEAN 105500.0f
#define IR_STD 1000.0f
#define RED_MEAN 101000.0f
#define RED_STD 800.0f

// Initialize TensorFlow Lite model
bool setupModel()
{
    // Set up logging
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Map the model into a usable data structure
    model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(error_reporter,
                             "Model provided is schema version %d not equal "
                             "to supported version %d.",
                             model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Pull in all the operation implementations we need
    static tflite::AllOpsResolver resolver;

    // Build an interpreter to run the model
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE, error_reporter);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
        return false;
    }

    // Get pointers to the model's input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);

    // Validate tensor dimensions
    if (input->dims->size != 3 ||
        input->dims->data[1] != WINDOW_SIZE ||
        input->dims->data[2] != 2 ||
        input->type != kTfLiteFloat32)
    {

        TF_LITE_REPORT_ERROR(error_reporter,
                             "Input tensor has incorrect dimensions or type");
        return false;
    }

    Serial.println("TensorFlow Lite model initialized successfully");
    return true;
}

// Run model inference on provided data
bool runInference(const DataBuffer *buffer, InferenceResult *result)
{
    if (!modelInitialized || !buffer || !result)
    {
        Serial.println("Cannot run inference - model not ready or invalid buffer");
        return false;
    }

    // Debug - print sample values for verification
    Serial.println("Sample raw values for verification:");
    for (int i = 0; i < 3 && i < WINDOW_SIZE; i++)
    {
        Serial.printf("  Raw[%d]: IR=%.2f, RED=%.2f\n",
                      i, buffer->buffer[i].ir, buffer->buffer[i].red);
    }

    // Use fixed values for normalization to match training data exactly
    Serial.printf("Using fixed normalization - IR_MEAN=%.2f, IR_STD=%.2f, RED_MEAN=%.2f, RED_STD=%.2f\n",
                  IR_MEAN, IR_STD, RED_MEAN, RED_STD);

    // Fill input tensor with standardized values using FIXED means/stds from training
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        // Standardize: (value - mean) / std using constants from training
        input->data.f[i * 2] = (buffer->buffer[i].ir - IR_MEAN) / IR_STD;
        input->data.f[i * 2 + 1] = (buffer->buffer[i].red - RED_MEAN) / RED_STD;
    }

    // Debug - print a few input values
    Serial.println("Model input sample (first 3 values - fixed standardization):");
    for (int i = 0; i < 3; i++)
    {
        Serial.printf("  Input[%d]: IR=%.6f, RED=%.6f\n",
                      i, input->data.f[i * 2], input->data.f[i * 2 + 1]);
    }

    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk)
    {
        Serial.println("Inference failed!");
        return false;
    }

    // Debug - print output tensor information
    Serial.printf("Output tensor - dims: %d, size: %d\n",
                  output->dims->size, output->dims->data[1]);

    // Process output
    float maxConfidence = -1.0f; // Start with negative so any valid value will be higher
    int maxIndex = 0;

    // Debug - print all output values
    Serial.println("Raw model output values:");
    int validOutputs = 0;
    float outputValues[3] = {0.0f, 0.0f, 0.0f}; // Store valid outputs here

    for (int i = 0; i < output->dims->data[1]; i++)
    {
        // Check for NaN or infinity
        if (isnan(output->data.f[i]) || isinf(output->data.f[i]))
        {
            Serial.printf("  Class %d: INVALID (NaN or Inf)\n", i);
            outputValues[i] = 0.0f; // Replace with 0 to avoid propagating NaN
        }
        else
        {
            outputValues[i] = output->data.f[i];
            Serial.printf("  Class %d: %.6f\n", i, outputValues[i]);
            validOutputs++;

            if (outputValues[i] > maxConfidence)
            {
                maxConfidence = outputValues[i];
                maxIndex = i;
            }
        }
    }

    // Apply softmax manually if we have at least one valid output
    if (validOutputs > 0)
    {
        // Find max for numerical stability
        float maxVal = outputValues[0];
        for (int i = 1; i < 3; i++)
        {
            if (outputValues[i] > maxVal)
                maxVal = outputValues[i];
        }

        // Apply exp and sum
        float expSum = 0.0f;
        float expValues[3];
        for (int i = 0; i < 3; i++)
        {
            expValues[i] = exp(outputValues[i] - maxVal);
            expSum += expValues[i];
        }

        // Calculate softmax probabilities
        Serial.println("Softmax probabilities:");
        maxConfidence = 0.0f;
        for (int i = 0; i < 3; i++)
        {
            float prob = expValues[i] / expSum;
            Serial.printf("  Class %d: %.6f\n", i, prob);
            if (prob > maxConfidence)
            {
                maxConfidence = prob;
                maxIndex = i;
            }
        }
    }
    else
    {
        // Try heuristic approach if model fails
        // Classify based on simple features extracted from the data
        float ir_var = 0, red_var = 0;
        float ir_mean = 0, red_mean = 0;

        // Calculate means
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            ir_mean += buffer->buffer[i].ir;
            red_mean += buffer->buffer[i].red;
        }
        ir_mean /= WINDOW_SIZE;
        red_mean /= WINDOW_SIZE;

        // Calculate variances
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            ir_var += (buffer->buffer[i].ir - ir_mean) * (buffer->buffer[i].ir - ir_mean);
            red_var += (buffer->buffer[i].red - red_mean) * (buffer->buffer[i].red - red_mean);
        }
        ir_var /= WINDOW_SIZE;
        red_var /= WINDOW_SIZE;

        Serial.printf("Heuristic features - IR variance: %.2f, RED variance: %.2f\n",
                      ir_var, red_var);

        // Use simple heuristic: high variance = walking, medium = sitting, low = resting
        float total_var = ir_var + red_var;
        if (total_var > 100000)
        {
            maxIndex = 2; // walking
            maxConfidence = 0.7f;
        }
        else if (total_var > 10000)
        {
            maxIndex = 1; // sitting
            maxConfidence = 0.6f;
        }
        else
        {
            maxIndex = 0; // resting
            maxConfidence = 0.5f;
        }

        Serial.println("WARNING: Using heuristic fallback classification");
        Serial.printf("Heuristic result: Class=%d, Confidence=%.2f\n", maxIndex, maxConfidence);
    }

    // Update result
    result->actionClass = maxIndex;
    result->confidence = maxConfidence;
    result->timestamp = millis();

    Serial.printf("Final inference result: Class=%d, Confidence=%.4f\n",
                  maxIndex, maxConfidence);
    return true;
}

#endif // MODEL_RUNNER_H
