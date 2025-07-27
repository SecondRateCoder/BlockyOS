#include <./src/Public/Publics.h>

#define MAXPRECISION sizeof(double)
// CORDIC algorithm.
/*
(Infinity)*45*(1/2X)


static void _cordic_calculate_sincos(double angle_rad, double *sin_val, double *cos_val) {
    double x = K; // Initial x-coordinate, scaled by the CORDIC gain
    double y = 0.0; // Initial y-coordinate
    double z = angle_rad; // Remaining angle to rotate

    double x_next, y_next;

    for (int i = 0; i < CORDIC_ITERATIONS; i++) {
        // Determine the direction of rotation
        int d = (z >= 0) ? 1 : -1;

        // Perform the CORDIC rotation
        x_next = x - d * y * pow(2.0, -i);
        y_next = y + d * x * pow(2.0, -i);
        z -= d * atan_table[i];

        x = x_next;
        y = y_next;
    }

    *cos_val = x;
    *sin_val = y;
}
*/
double sin(float angle){
	double out =0;
	for(int cc =0; cc < MAXPRECISION; ++cc){
		out += 45*(1/(2*angle));
	}
}

double cos(float angle){

}
float tan(float angle){return sin(angle)/cos(angle);}

ldouble_t pi(){

}

float powf(float val, const int pow){
	for(int cc= 0; cc < pow; cc++){val *= val;}
	return val;
}
size_t pow(size_t val, const int pow){
	for(int cc= 0; cc < pow; cc++){val *= val;}
	return val;
}