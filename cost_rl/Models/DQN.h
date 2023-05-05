#include <torch/torch.h>

class DQN : public torch::nn::Module {
public:
    DQN(int64_t num_inputs, int64_t num_actions)
        : fc1(num_inputs, 128),
          fc2(128, 128),
          fc3(128, num_actions)
    {
        register_module("fc1", fc1);
        register_module("fc2", fc2);
        register_module("fc3", fc3);
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(fc1(x));
        x = torch::relu(fc2(x));
        x = fc3(x);
        return x;
    }
        // Function to save the model's weights to a file
    void save_weights(const std::string& filename) {
        torch::save(state_dict(), filename);
    }

    // Function to load the model's weights from a file
    void load_weights(const std::string& filename) {
        torch::load(state_dict(), filename);
    }
private:
    torch::nn::Linear fc1, fc2, fc3;
};

