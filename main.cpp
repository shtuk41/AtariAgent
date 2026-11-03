#include <torch/torch.h>
#include <ale/ale_interface.hpp>
#include <iostream>
#include <cstdlib>
#include <vector>

struct DQN : torch::nn::Module {
    torch::nn::Conv2d conv1;
    torch::nn::Conv2d conv2;
    torch::nn::Conv2d conv3;
    torch::nn::Linear fc1;
    torch::nn::Linear fc2;

    explicit DQN(int num_actions)
        : conv1(torch::nn::Conv2dOptions(1, 32, 8).stride(4)),
        conv2(torch::nn::Conv2dOptions(32, 64, 4).stride(2)),
        conv3(torch::nn::Conv2dOptions(64, 64, 3).stride(1)),
        fc1(3136, 512),
        fc2(512, num_actions)
    {
        register_module("conv1", conv1);
        register_module("conv2", conv2);
        register_module("conv3", conv3);
        register_module("fc1", fc1);
        register_module("fc2", fc2);
    }

    torch::Tensor forward(torch::Tensor x) 
    {
        x = torch::relu(conv1(x));
        x = torch::relu(conv2(x));
        x = torch::relu(conv3(x));

        x = x.view({ x.size(0), -1 });

        //std::cout << x.sizes() << std::endl;
        
        x = torch::relu(fc1(x));

        return fc2(x);
    }
};

torch::Tensor get_processed_frame(
    ale::ALEInterface& ale,
    torch::Device device)
{
    const auto& screen = ale.getScreen();

    const int width = screen.width();
    const int height = screen.height();

    std::vector<uint8_t> grayscale(width * height);

    ale.getScreenGrayscale(grayscale);

    auto frame =
        torch::from_blob(
            grayscale.data(),
            { 1, 1, height, width },
            torch::kUInt8)
        .clone()
        .to(torch::kFloat32)
        .div(255.0f)
        .to(device);

    return torch::nn::functional::interpolate(
        frame,
        torch::nn::functional::InterpolateFuncOptions()
        .size(std::vector<int64_t>{84, 84})
        .mode(torch::kBilinear)
        .align_corners(false));
}

int main() {

    torch::Device device = torch::kCPU;

    if (torch::cuda::is_available()) {
        device = torch::Device(torch::kCUDA, 0);
        std::cout << "CUDA available\n";
    }
    else {
        std::cout << "CUDA unavailable, using CPU\n";
    }

    ale::ALEInterface ale;
    ale.setInt("random_seed", 42);
    ale.setBool("display_screen", true);
    ale.loadROM("pong.bin");

    ale::ActionVect legal_actions = ale.getMinimalActionSet();
    std::cout << "Successfully loaded Atari game!\n";
    std::cout << "Available distinct actions: " << legal_actions.size() << "\n";
    int num_actions = legal_actions.size();

    if (num_actions <= 0) {
        throw std::runtime_error("No legal Atari actions found.");
    }

    auto q_network = std::make_shared<DQN>(num_actions);

    q_network->to(device);

    std::cout
        << "Model device: "
        << q_network->parameters().front().device()
        << std::endl;

    torch::optim::Adam optimizer(q_network->parameters(), torch::optim::AdamOptions(1e-4));

    const float gamma = 0.99f;
    ale.reset_game();

    // Get initial state s on GPU
    torch::Tensor state = get_processed_frame(ale, device);

    // Training Loop for 500 Steps
    for (int step = 0; step < 50000; ++step) {
        if (ale.game_over()) {
            ale.reset_game();
            state = get_processed_frame(ale, device);
        }

        // Pass current state s through the network -> Q(s, a)
        torch::Tensor q_values = q_network->forward(state); // Shape: [1, num_actions]

        // Select action with highest Q-value (pull max value index back to CPU for indexing C++ vectors)
        int chosen_action_idx = q_values.argmax(1).cpu().item<int>();
        ale::Action ale_action = legal_actions[chosen_action_idx];

        // Store Q(s, a_chosen)
        torch::Tensor q_selected = q_values[0][chosen_action_idx];

        // Step environment to get reward r and next state s'
        float reward = static_cast<float>(ale.act(ale_action));
        bool is_terminal = ale.game_over();
        torch::Tensor next_state = get_processed_frame(ale, device);

        // Calculate Target Y using the network on s'
        torch::Tensor target_q;
        if (is_terminal) 
        {
            target_q = torch::tensor(
                { reward },
                torch::TensorOptions()
                .dtype(torch::kFloat32)
                .device(device));
        }
        else {
            torch::NoGradGuard no_grad; // Disable gradients for target estimation
            torch::Tensor next_q_values = q_network->forward(next_state);
            torch::Tensor q_max = std::get<0>(next_q_values.max(1));
            target_q = reward + (gamma * q_max);
        }

        // Compute MSE Loss: (Target - Prediction)^2
        torch::Tensor loss = torch::mse_loss(q_selected, target_q.squeeze(0));

        // Backpropagate and update network weights
        optimizer.zero_grad();
        loss.backward();
        optimizer.step();

        // Print progress periodically
        if (step % 50 == 0) {
            std::cout << "Step: " << step
                << " | Action: " << chosen_action_idx
                << " | Q_selected: " << q_selected.item<float>()
                << " | Reward: " << reward
                << " | Loss: " << loss.item<float>() << std::endl;
        }

        // Transition to next state
        state = next_state;
    }

    std::cout << "Completed 500 training steps." << std::endl;
    return 0;
}