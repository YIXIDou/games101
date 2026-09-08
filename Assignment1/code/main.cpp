#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f to_mat4(const Eigen::Matrix3f &m3)
{
    Eigen::Matrix4f m4 = Eigen::Matrix4f::Identity();
    m4.block<3, 3>(0, 0) = m3;
    return m4;
}

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

// TODO: Implement this function
// Create the model matrix for rotating the triangle around the Z axis.
// Then return it.
// 模型变换 (MVP - M)

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    float angle_rad = rotation_angle * MY_PI / 180.0f;
    model(0, 0) = std::cosf(angle_rad);
    model(0, 1) = -std::sinf(angle_rad);
    model(1, 0) = std::sinf(angle_rad);
    model(1, 1) = std::cosf(angle_rad);

    return model;
}

// 提升部分 要求:根据过原点的任意轴进行旋转
Eigen::Matrix4f get_rotation(Vector3f axis = Eigen::Vector3f(0.0f, 0.0f, 1.0f), float angle)
{
    float angle_rad = angle * MY_PI / 180.0f;
    Eigen::Vector3f n = axis.normalized();
    float nx = n.x();
    float ny = n.y();
    float nz = n.z();

    // 1. 构造反对称矩阵 K
    Eigen::Matrix3f K;
    K << 0.0f, -nz, ny,
        nz, 0.0f, -nx,
        -ny, nx, 0.0f;

    // 2. 利用矩阵公式计算 R = I + sin(theta)*K + (1 - cos(theta))*K^2
    Eigen::Matrix3f I = Eigen::Matrix3f::Identity();
    float cos_angle = std::cos(angle_rad);
    float sin_angle = std::sin(angle_rad);

    Eigen::Matrix3f R = I + sin_angle * K + (1.0f - cos_angle) * (K * K);
    return to_mat4(R);
}

// TODO: Implement this function
// Create the projection matrix for the given parameters.
// Then return it.
// 投影变换(MVP - P)

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function
    float angle_rad = eye_fov * MY_PI / 180;

    Eigen::Matrix4f ortho_translation = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f ortho_scale = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f perspective_to_ortho = Eigen::Matrix4f::Identity();

    float n = -zNear;
    float f = -zFar;
    float t = std::abs(n) * std::tanf(angle_rad / 2.0);
    float b = -t;
    float r = t * aspect_ratio;
    float l = -r;

    perspective_to_ortho(0, 0) = n;
    perspective_to_ortho(1, 1) = n;
    perspective_to_ortho(2, 2) = n + f;
    perspective_to_ortho(2, 3) = n * f * (-1.0f);
    perspective_to_ortho(3, 2) = 1.0f;
    perspective_to_ortho(3, 3) = 0.0f;

    ortho_translation(0, 3) = (-(l + r) / 2.0f);
    ortho_translation(1, 3) = (-(b + t) / 2.0f);
    ortho_translation(2, 3) = (-(n + f) / 2.0f);

    ortho_scale(0, 0) = 2.0f / (r - l);
    ortho_scale(1, 1) = 2.0f / (t - b);
    ortho_scale(2, 2) = 2.0f / (n - f);

    return ortho_scale *
           ortho_translation *
           perspective_to_ortho;
    // 注意矩阵运算没有交换律
    // 实际运算顺序为: perspective-to-orthographic → translation → scale
}

void _test();
// main函数存在三种输入模式
// ./Rasterizer                 循环运行程序 创建一个窗口显示 且你可以使用A键和D键旋转三角形   argc = 1
// ./Rasterizer −r 20           运行程序并将三角形旋转20度 然后将结果存在output.png中       argc = 3
// ./Rasterizer −r 20 image.png 运行程序并将三角形旋转20度 然后将结果存在image.png中        argc = 4

int main(int argc, const char **argv)
{

    // _test();
    // return 0;

    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3)
    {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4)
        {
            filename = std::string(argv[3]);
        }
        else if (argc > 4)
        {
            return 0;
        }
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a')
        {
            angle += 10;
        }
        else if (key == 'd')
        {
            angle -= 10;
        }
    }

    return 0;
}

void _test()
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    for (int i = 0; i < model.rows(); ++i)
    {
        for (int j = 0; j < model.cols(); ++j)
        {
            float val = model(i, j); // 访问第 i 行第 j 列
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    return;
}
