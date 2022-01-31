#include <drivers/adc.h>

typedef enum {
	LCZ_POWER_CONFIGURATION_DIRECT = 0,
	LCZ_POWER_CONFIGURATION_POTENTIAL_DIVIDER
} power_measure_configuration_t;

typedef struct {
	FwkMsgHeader_t header;
	uint8_t instance;
	power_measure_configuration_t configuration;
	enum adc_gain gain;
	float voltage;
} power_measure_msg_t;
