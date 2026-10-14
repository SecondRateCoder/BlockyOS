#include "SimpleOutput.h"

void SimpleCopyF(char *stream, uint64_t *size, const char **f, va_list ls){
    if(!stream || (*size) == 0 || !f || !ls){return;}
    static const char sample[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uint32_t counter = 0;
	const char *format = *f;
    
    //	Reserve 1 byte at the end for the null terminator (size - 1)
    for(uint32_t cc = 0; format[cc] != '\0' && counter < ((*size) - 1); ++cc){
        if(format[cc] == FormatStart){
            cc++; // Move past the start token
            if(format[cc] == '\0'){break;}
            uint64_t base = 10, value = 0;
            bool is_signed = false, negative = false;
            //	SAFELY EXTRACT VARIADIC ARGUMENTS
            // Smaller integer types (char, short) are automatically promoted to int/unsigned int by the C compiler.
            switch(format[cc]){
                case FormatUInteger8:	{value = (uint64_t)(uint8_t) va_arg(ls, unsigned int);	break;}
                case FormatUInteger16:	{value = (uint64_t)(uint16_t)va_arg(ls, unsigned int);	break;}
                case FormatUInteger32:	{value = (uint64_t)va_arg(ls, uint32_t);				break;}
                case FormatUInteger64:	{value = va_arg(ls, uint64_t);							break;}
                case FormatInteger8: {
                    int8_t temp = (int8_t)va_arg(ls, int);
                    if(temp < 0){negative = true;	value = (uint64_t)(-temp); }else{value = (uint64_t)temp;}
                    is_signed = true;
                    break;
                } case FormatInteger16: {
                    int16_t temp = (int16_t)va_arg(ls, int);
                    if(temp < 0){negative = true;	value = (uint64_t)(-temp);}else{value = (uint64_t)temp;}
                    is_signed = true;
                    break;
                } case FormatInteger32: {
                    int32_t temp = va_arg(ls, int32_t);
                    if(temp < 0){negative = true;	value = (uint64_t)(-temp);}else{value = (uint64_t)temp;}
                    is_signed = true;
                    break;
                } case FormatInteger64: {
                    int64_t temp = va_arg(ls, int64_t);
                    if(temp < 0){negative = true;	value = (uint64_t)(-temp);}else{value = (uint64_t)temp;}
                    is_signed = true;
                    break;
                } case FormatString: {
                    const char *str = va_arg(ls, const char *);
                    if(!str){str = "(null)";}
                    // Correctly track the string copy lengths
                    while(*str && counter < ((*size) - 1)){stream[counter++] = *str++;}
                    continue; // Skip the integer rendering logic below
                } case FormatChar: {
                    stream[counter++] = (char)va_arg(ls, int);
                    continue; 
                } default: {stream[counter++] = format[cc];        continue;}//	If the token is unrecognized, print it as a literal
				
            }

            //	FORMAT INTEGER TO TEMPORARY BUFFER
            //	Modulo arithmetic extracts the LEAST significant digit first. 
            //	We store it in a temp buffer so we can reverse it.
            char num_buf[70]; 
            int num_len = 0;

			//	Handle explicit zero
            if(value == 0){num_buf[num_len++] = '0';}else{
                while(value > 0){
                    num_buf[num_len++] = sample[value % base];
                    value /= base;
                }
            }
            if(negative && is_signed){num_buf[num_len++] = '-';}

            //	COPY TO STREAM IN REVERSE ORDER
            for(int i = num_len - 1; i >= 0 && counter < ((*size) - 1); i--){stream[counter++] = num_buf[i];}

		//	Normal character processing
        }else{stream[counter++] = format[cc];}
    }

    //	GUARANTEE NULL TERMINATION
    stream[counter] = '\0';
	*size = counter;
    
    va_end(ls);

	*f = format;
}

void DebugPrintf(const char *f, ...){
	va_list	ls;	va_start(ls, f);
	static __linkersection(.bss) char stream[1024] = {0};
	memset(stream, 0, sizeof(stream));
	uint64_t size = sizeof(stream);
	SimpleCopyF(stream, &size, &f, ls);
	e9print(stream, size);
}