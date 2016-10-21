//
//  TrackingUtil.m
//  CardioFace
//
//

#include "TrackingUtil.h"
// 所有的梯度结果 * 2
void image_gradient_color_x2(const BGRAColor* matrix, const int imgWidth, const int imgHeight,
                             const int X, const int Y, const int width, const int height,
                             float* dx, float* dy) {
    // 实现算法参考Matlab: gradient
    // 1. boundary handling
    // first colum and last colum
    // dx: 水平方向的梯度的计算(左右两个边界)
    int roiOffset = Y * imgWidth + X;
    int offset = roiOffset + 0;
    int i, k, pos;

    for (i = 0; i < height; ++i) {
        dx[offset]     = ((int)matrix[offset + 1].red - (int)matrix[offset].red) * 2;
        offset += imgWidth;

        dx[offset - 1] = ((int)matrix[offset - 1].red - (int)matrix[offset - 2].red) * 2;
    }

    // boundary handling
    // first row and last row(上下两个边界)
    offset = (height - 1) * imgWidth + roiOffset;
    for (i = 0; i < width; ++i) {
        dy[i]          = ((int)matrix[i + imgWidth].red - (int)matrix[i].red) * 2;

        const int offseti = offset + i;
        dy[offseti] = ((int)matrix[offseti].red   - (int)matrix[offseti - imgWidth].red) * 2;
    }

    // dx(i, k) = (matrix(i, k - 1) + matrix(i, k + 1)) - 2 * matrix(i, k)
    // dy(i, k) = (matrix(i - 1, k) + matrix(i + 1, k)) - 2 * matrix(i, k)

    const uint32_t* const matrix1 = (const uint32_t*)matrix;

    offset = 0 + roiOffset;
    const int width_1 = width - 1;
    for (i = 0; i < height; ++i) {
        // 忽略两个边界
        for (k = 1; k < width_1; ++k) {
            const int pos = offset + k;
            dx[pos] = RED_COMP(matrix1[pos + 1]) - RED_COMP(matrix1[pos - 1]); //  * 0.5;
        }
        offset += imgWidth;
    }

    offset = 0 + roiOffset;
    const int height_1 = height - 1;
    for (i = 1; i < height_1; ++i) {
        offset += imgWidth;
        for (k = 0; k < width; ++k) {
            pos = offset + k;
            dy[pos] = RED_COMP(matrix1[pos + imgWidth]) - RED_COMP(matrix1[pos - imgWidth]); //  * 0.5;
        }
    }
}

void image_gradient_int_x2(const uint8_t* matrix, const int width, const int height, int* dx, int* dy) {
    // 实现算法参考Matlab: gradient
    // 1. boundary handling
    // first colum and last colum
    // dx: 水平方向的梯度的计算(左右两个边界)
    int offset = 0;
    int i, k;
    for (i = 0; i < height; ++i) {
        dx[offset]     = ((int)matrix[offset + 1] - (int)matrix[offset]) * 2;
        offset += width;

        dx[offset - 1] = ((int)matrix[offset - 1] - (int)matrix[offset - 2]) * 2;
    }

    // boundary handling
    // first row and last row(上下两个边界)
    offset = (height - 1) * width;
    for (i = 0; i < width; ++i) {
        dy[i]          = ((int)matrix[i + width] - (int)matrix[i]) * 2;

        dy[offset + i] = ((int)matrix[offset + i] - (int)matrix[offset + i - width]) * 2;
    }

    // dx(i, k) = (matrix(i, k - 1) + matrix(i, k + 1)) - 2 * matrix(i, k)
    // dy(i, k) = (matrix(i - 1, k) + matrix(i + 1, k)) - 2 * matrix(i, k)
    offset = 0;
    for (i = 0; i < height; ++i) {
        // 忽略两个边界
        for (k = 1; k < width - 1; ++k) {
            dx[offset + k] = ((int)matrix[offset + k + 1] - (int)matrix[offset + k - 1]);
        }
        offset += width;
    }

    offset = 0;
    for (i = 1; i < height - 1; ++i) {
        offset += width;
        for (k = 0; k < width; ++k) {
            dy[offset + k] = ((int)matrix[offset + k + width] - (int)matrix[offset + k - width]);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void image_gradient_x2(const uint8_t* matrix, const int width, const int height, float* dx, float* dy) {


    // 实现算法参考Matlab: gradient
    // 1. boundary handling
    // first colum and last colum
    // dx: 水平方向的梯度的计算(左右两个边界)
    int offset = 0;
    int i, k;
    for (i = 0; i < height; ++i) {
        dx[offset]     = ((int)matrix[offset + 1] - (int)matrix[offset]) * 2;
        offset += width;

        dx[offset - 1] = ((int)matrix[offset - 1] - (int)matrix[offset - 2]) * 2;
    }

    // boundary handling
    // first row and last row(上下两个边界)
    offset = (height - 1) * width;
    for (i = 0; i < width; ++i) {
        dy[i]          = ((int)matrix[i + width] - (int)matrix[i]) * 2;

        dy[offset + i] = ((int)matrix[offset + i] - (int)matrix[offset + i - width]) * 2;
    }

    // dx(i, k) = (matrix(i, k - 1) + matrix(i, k + 1)) - 2 * matrix(i, k)
    // dy(i, k) = (matrix(i - 1, k) + matrix(i + 1, k)) - 2 * matrix(i, k)
    offset = 0;
    for (i = 0; i < height; ++i) {
        // 忽略两个边界
        for (k = 1; k < width - 1; ++k) {
            dx[offset + k] = ((int)matrix[offset + k + 1] - (int)matrix[offset + k - 1]); //  * 0.5;
        }
        offset += width;
    }

    offset = 0;
    for (i = 1; i < height - 1; ++i) {
        offset += width;
        for (k = 0; k < width; ++k) {
            dy[offset + k] = ((int)matrix[offset + k + width] - (int)matrix[offset + k - width]); // * 0.5;
        }
    }
}

/***
 *　输入参数:
 *   1. 在本系统中inData可能为　dx, dy, IxIy, 它们都是int32_t, 且范围在+/- 255^2
 *   2. 时间复杂度为: 2 * imgWidth * imgHeight * 5, 而: image_gussian_filter的时间复杂度为: 5 * 5 * imgWidth * imgHeight
 *   3. 底层运算: 所有的运算都采用int32_t的乘法，移位，以及加减，避免浮点运算，和vDSP_f5x5更加高效
 */
void image_gussian_filter_5(int32_t* inData, int32_t* outData, int32_t*buffer,
                            const int imgWidth, const int imgHeight) {
    // 1, 4, 6, 4, 1
    // X方向滤波
	int i, j, index;
    const int imgWidth_2 = imgWidth - 2;

    for (i = 0; i < imgHeight; i++) {
        // 行首两个元素
        buffer[index] = inData[index] * 6 + (inData[index + 1] << 2)                     + inData[index + 2]; index++;
        buffer[index] = inData[index] * 6 + ((inData[index+1] + inData[index - 1]) << 2) + inData[index + 2]; index++;
        for (j = 2; j < imgWidth_2; j++) {
            buffer[index] = inData[index] * 6 + ((inData[index+1] + inData[index - 1]) << 2)
                + (inData[index+2] + inData[index - 2]);
            index++;
        }

        buffer[index] = inData[index] * 6 + ((inData[index+1] + inData[index - 1]) << 2) + inData[index - 2]; index++;
        buffer[index] = inData[index] * 6 + (inData[index - 1] << 2)                     + inData[index - 2]; index++;

    }

    // Y方向滤波
    const int imgHeight_2 = imgHeight - 2;
    const int imgWidth2 = imgWidth << 1;

    for (j = 0; j < imgWidth; j++) {
        index = j;

        outData[index] = buffer[index] * 6 + (buffer[index + imgWidth] << 2) + buffer[index + imgWidth2];
        index += imgWidth;

        outData[index] = buffer[index] * 6 + ((buffer[index - imgWidth] + buffer[index + imgWidth]) << 2)
                       + buffer[index + imgWidth2];
        index += imgWidth;


        for (i = 2; i < imgHeight_2; i++) {
            outData[index] = buffer[index] * 6 + ((buffer[index - imgWidth] + buffer[index + imgWidth]) << 2)
                           + buffer[index - imgWidth2] + buffer[index + imgWidth2];
            index += imgWidth;
        }
        outData[index] = buffer[index] * 6 + ((buffer[index - imgWidth] + buffer[index + imgWidth]) << 2)
                       + buffer[index - imgWidth2];
        index += imgWidth;

        outData[index] = buffer[index] * 6 + ((buffer[index - imgWidth]) << 2)
                       + buffer[index - imgWidth2];
        // index += imgWidth;
    }

}

void matrix22_inv(const float* matrix, float* retMatrix) {

    double det = matrix[0] * matrix[3] - matrix[1] * matrix[2];

    // 矩阵必须可逆
    TTDASSERT(fabs(det) > 1e-5);

    double inv_det = 1.0 / det;

    retMatrix[0] =   matrix[3] * inv_det;
    retMatrix[1] =  -matrix[1] * inv_det;

    retMatrix[2] =  -matrix[2] * inv_det;
    retMatrix[3] =   matrix[0] * inv_det;
}
