#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

using namespace std;
using namespace cv;

// Saving results for visualization
vector<Mat> Laplacian_pyramid_src_1, Laplacian_pyramid_src_2, Laplacian_pyramid_blended, Reconstruct_pyramid;

// Getting Laplacian pyramid of an image. 
// Notice: the result vector contains < Lap_pyr_1, ... Lap_pyr_level, Gaussian_pyr_level+1 >
vector<Mat> get_Laplacian_pyramid(int level_num, Mat& src){
    
    vector<Mat> Laplacian_pyramid;

    Mat tmp; src.copyTo(tmp);
    for(int i = 0; i < level_num; i++){
        Mat Gaussian_pyramid_i,_up, Laplacian_pyramid_i;
        pyrDown(tmp, Gaussian_pyramid_i);
        pyrUp(Gaussian_pyramid_i, _up, tmp.size());
        Laplacian_pyramid_i = tmp - _up;
        Laplacian_pyramid.push_back(Laplacian_pyramid_i);
        Gaussian_pyramid_i.copyTo(tmp);
    }
    Laplacian_pyramid.push_back(tmp);
    return Laplacian_pyramid;
}
// Getting blended Laplacian pyramid.
// blended_Laplacian_pyramid_i = pyr1_i * mask1 + pyr2_i * mask2
// Notice: The result vector contains < blended_Lap_pyr_1, ... blended_Lap_pyr_level, blended_Gaussian_pyr_level+1 >
vector<Mat> get_blended_Laplacian_pyramid(int level_num, vector<Mat>pyr1, vector<Mat>pyr2, Mat& mask1, Mat& mask2){
    vector<Mat> blended_Laplacian_Pyramid;
    Mat mask1_tmp, mask2_tmp; mask1.copyTo(mask1_tmp); mask2.copyTo(mask2_tmp);
    Mat mask1_BGR, mask2_BGR;
    cvtColor(mask1_tmp, mask1_BGR, COLOR_GRAY2BGR);
    cvtColor(mask2_tmp, mask2_BGR, COLOR_GRAY2BGR);

    for(int i = 0 ; i <=level_num; i++){//0 ~ level_num-1 is Laplacian pyramid. level_num is gaussian pyramid_level_num+1
        Mat Lap_pyr_1_i = pyr1[i].mul(mask1_BGR); Mat Lap_pyr_2_i = pyr2[i].mul(mask2_BGR);
        Mat blended_Lap_pyr_i = Lap_pyr_1_i + Lap_pyr_2_i;

        blended_Laplacian_Pyramid.push_back(blended_Lap_pyr_i);

        pyrDown(mask1_tmp,mask1_tmp);
        pyrDown(mask2_tmp,mask2_tmp);
        cvtColor(mask1_tmp, mask1_BGR, COLOR_GRAY2BGR);
        cvtColor(mask2_tmp, mask2_BGR, COLOR_GRAY2BGR);
    }
    return blended_Laplacian_Pyramid;
}
// Main blending function
Mat Img_blending_based_on_Laplacian_pyr(int level, Mat& src1, Mat& src2, Mat& mask1, Mat& mask2){
    vector<Mat> Lap_pyr_1, Lap_pyr_2, Lap_pyr_blended;
    Laplacian_pyramid_src_1 = Lap_pyr_1 = get_Laplacian_pyramid(level, src1);
    Laplacian_pyramid_src_2 = Lap_pyr_2 = get_Laplacian_pyramid(level, src2);
    Laplacian_pyramid_blended = Lap_pyr_blended = get_blended_Laplacian_pyramid(level,Lap_pyr_1,Lap_pyr_2,mask1,mask2);

    Mat current_Img = Lap_pyr_blended.back(); // Gaussian_Pyramid_i+1
    // Reconstructing
    for(int i = level-1; i >= 0; i--){
        Mat _up;
        pyrUp(current_Img, _up, Lap_pyr_blended[i].size());

        Mat resconstructed_Img;
        resconstructed_Img = _up + Lap_pyr_blended[i];

        resconstructed_Img.copyTo(current_Img);
        Reconstruct_pyramid.push_back(resconstructed_Img);
    }
    current_Img.convertTo(current_Img,CV_8UC3); 
    return current_Img;
}
// Visualization function
// Given a vector and a window name, generate a concatenated image and show it.
// Notice: If you want to visualize Laplacian pyramid of src1, src2 or blended,
// you need to convert images in vectors to CV_8UC3, otherwise a wrong black img will be shown.
void Visualization_vector(vector<Mat>src, string window_name, bool save = false){
    //
    int show_res_width = 0, show_res_height = 0;

    int size = src.size();
    for(int i = 0; i < size; i++){
        Mat current_Img = src[i];
        show_res_width += current_Img.size().width;
        show_res_height = max(show_res_height, current_Img.size().height);
    }
    
    Mat show_res = Mat::zeros(Size(show_res_width,show_res_height),CV_8UC3);
    int current_width = 0;
    for(int i = 0; i < size; i++){
        Mat current_Img = src[i];
        Rect show_rect(current_width,0,current_Img.size().width,current_Img.size().height);
        current_Img.copyTo(show_res(show_rect));
        current_width += current_Img.size().width;
    }
    imshow(window_name,show_res);
    if(save == true){
        imwrite("./data/" + window_name + ".jpg",show_res);
    }
    waitKey(0);
}
int main(void)
{
    string path1 = "./data/apple_big.jpg";
    string path2 = "./data/orange_big.jpg";
    Mat src_image_1 = imread(path1);
    Mat src_image_2 = imread(path2);

    Mat src_image_1_32F, src_image_2_32F;
    src_image_1.convertTo(src_image_1_32F,CV_32FC3);
    src_image_2.convertTo(src_image_2_32F,CV_32FC3);

    Mat mask1 = Mat::zeros(src_image_1.size(), CV_32FC1);
    mask1(Range::all(), Range(0, mask1.cols * 0.5)) = 1.0;

    Mat mask2 = Mat::zeros(src_image_2.size(),CV_32FC1);
    mask2.setTo(1.0);
    mask2 -= mask1;

    int level = 10;
    Mat res = Img_blending_based_on_Laplacian_pyr(level,src_image_1_32F,src_image_2_32F,mask1,mask2);
    vector <Mat> res_vec; res_vec.push_back(src_image_1); res_vec.push_back(src_image_2); res_vec.push_back(res);
    string window_name = "level=" + to_string(level) + "_result";
    
    Visualization_vector(res_vec,window_name,true);
    Visualization_vector(Reconstruct_pyramid,"Reconstruct_pyramid",true);
    return 0;
}


