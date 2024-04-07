#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#pragma warning(disable:4996)

unsigned char SWUFP8Add(unsigned char swnum1, unsigned char swnum2);
unsigned char SWUFP8Sub(unsigned char swnum1, unsigned char swnum2);

unsigned char SWUFP8Add(unsigned char swnum1, unsigned char swnum2) {
    // 부호 추출
    int sign1 = (swnum1 & 0x80) >> 7;
    int sign2 = (swnum2 & 0x80) >> 7;

    // 지수와 가수 부분 추출
    int exp1 = (swnum1 >> 4) & 0x07;
    int exp2 = (swnum2 >> 4) & 0x07;
    int mant1 = (swnum1 & 0x0F) | 0x10;
    int mant2 = (swnum2 & 0x0F) | 0x10;

    // 지수 정렬

    while (exp1 > exp2) {
        mant2 >>= 1;
        exp2++;
    }
    while (exp2 > exp1) {
        mant1 >>= 1;
        exp1++;
    }

    // 가수 부분 더하기
    int mantResult = (sign1 ? -mant1 : mant1) + (sign2 ? -mant2 : mant2);
    int resultSign = mantResult < 0 ? 1 : 0;

    // 오버플로우와 언더플로우 처리
    if (mantResult < 0) mantResult = -mantResult; // 결과의 절대값
    while (mantResult & 0x20) {
        mantResult >>= 1;
        exp1++;
    }
   
    while (!(mantResult & 0x10) && exp1 > 0) {
        mantResult <<= 1;
        exp1--;
    }

  

    // 결과 조합
    mantResult &= 0x0F;

    unsigned char result = (resultSign << 7) | (exp1 << 4) | mantResult;
    return result;
}

unsigned char SWUFP8Sub(unsigned char swnum1, unsigned char swnum2) {
    int sign1 = (swnum1 & 0x80) >> 7;
    int sign2 = ((swnum2 & 0x80) >> 7) ^ 1;

    // 지수와 가수 부분 추출
    int exp1 = (swnum1 >> 4) & 0x07;
    int exp2 = (swnum2 >> 4) & 0x07;
    int mant1 = (swnum1 & 0x0F) | 0x10; // 묵시적으로 1을 추가
    int mant2 = (swnum2 & 0x0F) | 0x10; // 묵시적으로 1을 추가

    // 지수 정렬
    int shift = exp1 - exp2;
    if (shift > 0) {
        mant2 >>= shift; // 지수가 더 큰 수에 맞춤
        exp2 += shift;
    }
    else {
        mant1 >>= -shift;
        exp1 = exp2; // 지수를 더 큰 쪽에 맞춤
    }

    // 가수 부분 뺄셈
   // int mantResult = mant1 - mant2;
    int mantResult = (sign1 ? -mant1 : mant1) + (sign2 ? -mant2 : mant2);
    int resultSign = mantResult < 0 ? 1 : 0;


    // 결과 정규화


    // 가수 부분 뺄셈 후 정규화 및 오버플로우, 언더플로우 처리
    if (mantResult < 0) {
        mantResult = -mantResult; // 음수 결과에 대해 절대값 취함
    }

    // 오버플로우 처리: 가수가 범위를 초과하는 경우 지수를 증가시킴
    while (mantResult & 0x20) {
        mantResult >>= 1;
        exp1++;
    }

    // 언더플로우 처리: 가수가 범위 미만인 경우 지수를 감소시킴
    while (!(mantResult & 0x10) && exp1 > 0) {
        mantResult <<= 1;
        exp1--;
    }


    // 결과 조합
    mantResult &= 0x0F;
    unsigned char result = (resultSign << 7) | (exp1 << 4) | mantResult;
    return result;
}


char floatToFP(float value) {
   
    char result = 0;
    char sign = 0, exponent = 0;
    char mantissa = 0;

    // 부호 결정: 양수면 0, 음수면 1
    if (value < 0) {
        sign = 1;
        value = -value; // 값이 음수면 양수로 변환
    }

    // 지수와 가수 계산
    if (value >= 1.0) {
        // 값이 1.0 이상일 때, 2로 나누면서 지수 증가
        while (value >= 2.0) {
            value /= 2.0;
            exponent++;
        }
    }
    else {
        // 값이 1.0 미만일 때, 2를 곱하면서 지수 감소
        while (value < 1.0 && exponent > -7 ) {         // 수정 !!!!
            value *= 2.0;
            exponent--;
        }
    }
    
    exponent += 3; // 지수에 +3을 해야 함

    value *= 16; // 2^4로 스케일링
    mantissa = (char)value & 0x0F; // 상위 4비트만 취함

    // 비트 배치
    sign = sign << 7; // 가장 첫 번째 비트로
    exponent = (exponent & 0x07) << 4; // 다음 3비트로, 오버플로 방지를 위해 마스킹
    // 가수부는 이미 하위 4비트에 위치

    result = sign | exponent | mantissa;

    return result;
}

float FPToFloat(unsigned char value) {

    // 부호 비트 추출 (최상위 비트)
    int sign = (value & 0x80) ? -1 : 1;
 ;
    // 지수 부분 추출 (3비트) 및 실제 지수 값 계산
    int exponent;
    float mantissa;
    if (value == 0) {
        exponent = ((value >> 4) & 0x07);
        mantissa = ((value & 0x0F) / 16.0f);
    }
    else {
        exponent = ((value >> 4) & 0x07) - 3;
        mantissa = 1.0f + ((value & 0x0F) / 16.0f);
    }
   
    // 가수 부분 추출 (하위 4비트) 및 1.m 형태로 변환

   
    // 실수 값 계산: (-1)^sign * 1.m * 2^exponent
    float realValue = sign * mantissa * (float)pow(2, exponent);
 
    return realValue;
}

int main(void) {
    char fop;
    float fnum1, fnum2, fresult;
    unsigned char swufnum1, swufnum2, swuresult = 0;


    /* read operator float number1 float number2 */
    

   for(;;){
    scanf(" %c", &fop);
    if (fop == 'E') {
        return 0;
    }
    
    scanf("%f %f", &fnum1, &fnum2);

    /* translate floating number to SWUFP8 */
    swufnum1 = floatToFP(fnum1);
    swufnum2 = floatToFP(fnum2);
   

    /* if fop is ‘+’ */
    if (fop == '+') {
        swuresult = SWUFP8Add(swufnum1, swufnum2);
    }

    /* else if fop is ‘-‘ */
    else if (fop == '-') {
        swuresult = SWUFP8Sub(swufnum1, swufnum2);
    }
    
    /* else terminate */
    if (fop != '+' &&  fop != '-' ) {
        printf("Not supported. Try again.\n");
        continue; 
    }
    
  
    /* Conversion SWU-FP8 type to float */
    fresult = FPToFloat(swuresult);
     
   
    /* object code = {Zero, DeN, InF, NaN} */
    /* print operator float number1 float number2 object-code */
    if (fresult == 0) {
       printf("%c %.1f %.1f %.1f", fop, fnum1, fnum2, fresult);
       printf(" Zero \n");
    }
    else printf("%c %.1f %.1f %.1f \n", fop, fnum1, fnum2, fresult);
  }
   
    return 0; 
}

