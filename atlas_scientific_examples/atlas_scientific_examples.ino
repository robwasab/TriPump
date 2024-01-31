#include <Wire.h>
#include <string.h>

#define ASSERT(x) do {               \
  if(!(x)) {                         \
    Serial.print("Assertion, line:");\
    Serial.print(__LINE__);          \
    Serial.print(" ");               \
    Serial.println(__FUNCTION__);    \
    while(1);                        \
  }                                  \
} while(0)

enum PUMP_ADDR
{
  PUMP_ADDR_0 = 0x38,
  PUMP_ADDR_1 = 0x39,
  PUMP_ADDR_2 = 0x3A,
};


bool send_cmd(
  enum PUMP_ADDR addr, 
  uint8_t cmd_data[], 
  size_t cmd_len,
  uint8_t rsp_data[],
  size_t rsp_len)
{
  ASSERT(cmd_data != NULL);
  ASSERT(0 < cmd_len);
  
  Wire.beginTransmission(addr);

  Wire.write(cmd_data, cmd_len);

  Wire.endTransmission(true);

  // per datasheet
  delay(300);

  size_t num2request = 1 + rsp_len;

  Wire.requestFrom(addr, num2request, true);

  size_t num_available = Wire.available();

  if(num_available == 0) {
    Serial.println("Wire.requestFrom() failed...");
    return false;
  }
  else {
    uint8_t status = Wire.read();
    //Serial.print("Status code: ");
    //Serial.println(status, HEX);

    for(size_t k = 0; k < rsp_len; k++)
    {
      ASSERT(NULL != rsp_data);
      rsp_data[k] = Wire.read();
    }

    if(status != 1) {
      Serial.print("Wire.requestFrom() -> 0x");
      Serial.println(status, HEX);
    }
    while(Wire.available()) {
      Wire.read();
    }
    return status == 1;
  }

}

bool get_fw_version(int * fw_major, int * fw_minor)
{
  ASSERT(NULL != fw_major);
  ASSERT(NULL != fw_minor);

  char cmd[] = "i";
  
  char buf[64] = {0};
  memset(buf, 0, sizeof(buf));

  bool status = send_cmd(PUMP_ADDR_0, cmd, strlen(cmd), buf, 16);

  //Serial.println(buf);

  sscanf(buf, "?I,PMP,%d.%d", fw_major, fw_minor);

  Serial.print("fw major: ");
  Serial.println(*fw_major);

  Serial.print("fw minor: ");
  Serial.println(*fw_minor);

  return status;
}

void get_dispense_status(enum PUMP_ADDR addr)
{
  char buf[64] = {0};
  char cmd[] = "D,?";

  bool status = send_cmd(addr, cmd, strlen(cmd), buf, 16);
  
  float pump_on = -1;
  float last_volume = -1;
  Serial.println(buf);
  sscanf(buf, "?D,%f,%f", &last_volume, &pump_on);

  Serial.print("last volume request: ");
  Serial.println(last_volume);

  Serial.print("pump on: ");
  Serial.println(pump_on);
}

void dispense(enum PUMP_ADDR addr)
{
  char cmd[] = "D,*";

  bool status = send_cmd(addr, cmd, strlen(cmd), NULL, 0);
}

void stop_dispensing(enum PUMP_ADDR addr)
{
  char cmd[] = "x";
  
  char buf[64] = {0};
  memset(buf, 0, sizeof(buf));

  bool status = send_cmd(addr, cmd, strlen(cmd), buf, 16);

  Serial.println(buf);
  
}

void dispense_amount(enum PUMP_ADDR addr, float amt)
{
  char cmd_buf[64] = {0};
  
  sprintf(cmd_buf, "D,%.3f", amt);

  bool status = send_cmd(addr, cmd_buf, strlen(cmd_buf), NULL, 0);
}

void setup() {
  // put your setup code here, to run once:

  Wire.begin();        // join I2C bus (address optional for master)
  Serial.begin(115200);  // start serial for output


}

void loop() {

  // put your main code here, to run repeatedly:
  int fw_major = -1;
  int fw_minor = -1;
  
  get_fw_version(&fw_major, &fw_minor);

  get_dispense_status(PUMP_ADDR_0);

  //dispense_amount(PUMP_ADDR_0, 10);

  static int index = 0;
  enum PUMP_ADDR addrs[] = {PUMP_ADDR_0, PUMP_ADDR_1, PUMP_ADDR_2};

  enum PUMP_ADDR addr = addrs[index];
  index = (index + 1) % 3;

  Serial.print("Dispensing index: ");
  Serial.println(index);

  dispense(addr);
  delay(1000);
  stop_dispensing(addr);
}
