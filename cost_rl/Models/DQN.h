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

private:
    torch::nn::Linear fc1, fc2, fc3;
};

