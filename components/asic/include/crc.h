#ifndef CRC_H_
#define CRC_H_

static extern int crc_fail_count;
uint8_t crc5(uint8_t *data, uint8_t len, int offset);
unsigned short crc16(const unsigned char *buffer, int len);
unsigned short crc16_false(const unsigned char *buffer, int len);
void create_buf_for_crc_test(const asic_result *result, unsigned char *array);

#endif // PRETTY_H_