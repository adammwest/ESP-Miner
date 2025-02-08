#ifndef CRC_H_
#define CRC_H_

extern int crc_fail_count;
uint8_t crc5(uint8_t *data, uint8_t len, int offset);
unsigned short crc16(const unsigned char *buffer, int len);
unsigned short crc16_false(const unsigned char *buffer, int len);
bool crc_test(const unsigned char *data, int buf_len);
#endif // PRETTY_H_