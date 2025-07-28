#include <./src/Public/Publics.h>

#define MAXPRECISION 32
rad_deg_t *arctan;

void arctan_init(){
    arctan = alloca(MAXPRECISION, KERNEL_ID);
    ldouble_t _45 = 45, _rad = 0.785398163;
    for(int cc =0; cc < MAXPRECISION; cc++){
        arctan[cc] = (rad_deg_t){_45, _rad};
        _45 >> 2;
        _rad >> 2;
    }
}
// CORDIC algorithm.
/*
(Infinity)*45*(1/2X)


static void _cordic_calculate_sincos(double angle_rad, double *sin_val, double *cos_val) {
    double x = K; // Initial x-coordinate, scaled by the CORDIC gain
    double y = 0.0; // Initial y-coordinate
    double angle = angle_rad; // Remaining angle to rotate

    double x_next, y_next;

    for (int i = 0; i < CORDIC_ITERATIONS; i++) {
        // Determine the direction of rotation
        int d = (angle >= 0) ? 1 : -1;

        // Perform the CORDIC rotation
        x_next = x - d * y * pow(2.0, -i);
        y_next = y + d * x * pow(2.0, -i);
        angle -= d * atan_table[i];

        x = x_next;
        y = y_next;
    }

    *cos_val = x;
    *sin_val = y;
}
*/
double cordic_k(){
    double k =0;
    for(int i =0; i < MAXPRECISION; ++i){
        k *= sqrt(1.0 + pow(2.0, -2.0 * i));
    }
    return k;
}

double *sincos(double rad_angle){

} 

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

size_t power(size_t base, size_t exp, size_t mod) {
    size_t res = 1;
    base %= mod; // Ensure base is within modulo range
    while (exp > 0) {
        if (exp % 2 == 1) { // If exponent is odd, multiply result by base
            res = (res * base) % mod;
        }
        base = (base * base) % mod; // Square the base
        exp /= 2; // Halve the exponent
    }
    return res;
}


double cordic_arctan(double y_in, double x_in) {
    double x = x_in;
    double y = y_in;
    double angle = 0.0;

    // Handle trivial cases to avoid division by zero or incorrect quadrant for CORDIC core
    if (x == 0.0 && y == 0.0) return 0.0; // Undefined, or 0 if magnitude is 0
    if (x == 0.0) return (y > 0) ? M_PI / 2.0 : -M_PI / 2.0; // Straight up or down

    // Normalize input to the CORDIC primary range for vectoring mode
    // This typically means ensuring x is positive.
    // We'll use a quadrant tracking similar to sincos for robustness.
    int quadrant_adjust = 0;
    if (x_in < 0) {
        if (y_in >= 0) quadrant_adjust = 1; // Q2
        else quadrant_adjust = 2; // Q3
        x = -x_in; // Make x positive for CORDIC core
        y = -y_in; // Adjust y accordingly
    } else { // x_in >= 0
        if (y_in < 0) quadrant_adjust = 3; // Q4
        // Q1 (x_in >= 0, y_in >= 0) needs no initial adjustment
    }

    // CORDIC iterations
    for (int i = 0; i < NUM_ITERATIONS_ARCTAN; ++i) {
        double x_old = x;
        double y_old = y;
        int sigma; // Direction of rotation: +1 for clockwise, -1 for counter-clockwise

        // In vectoring mode, sigma is chosen to drive y towards zero.
        if (y_old > 0) {
            sigma = 1; // If y is positive, rotate clockwise to decrease y.
            x = x_old + (y_old * pow(2.0, -i));
            y = y_old - (x_old * pow(2.0, -i));
            angle = angle + atan_table_arctan[i]; // Accumulate the angle
        } else { // y_old <= 0
            sigma = -1; // If y is negative or zero, rotate counter-clockwise to increase y (make it less negative).
            x = x_old - (y_old * pow(2.0, -i));
            y = y_old + (x_old * pow(2.0, -i));
            angle = angle - atan_table_arctan[i]; // Accumulate the angle
        }
    }

    // Adjust the accumulated angle based on the original quadrant
    switch (quadrant_adjust) {
        case 1: // Q2: angle = PI - angle
            return M_PI - angle;
        case 2: // Q3: angle = PI + angle (angle is negative, so PI - |angle|)
            return M_PI + angle; // angle is already negative from CORDIC, so this adds the magnitude
        case 3: // Q4: angle = 2*PI - angle (angle is negative, so 2*PI + |angle|)
            return 2 * M_PI - angle; // angle is already negative from CORDIC, so this adds the magnitude
        default: // Q1: angle is already correct
            return angle;
    }
}