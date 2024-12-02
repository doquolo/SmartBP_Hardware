// Define a struct to hold the parsed data
struct Record
{
  int sys;              // Systolic pressure
  int dia;              // Diastolic pressure
  int pulse;            // Heart rate
  int mean;             // Mean pressure
  int heat;             // Heat flag
  char record_time[20]; // Timestamp in "YYYY-MM-DD HH:MM:SS" format
};

// Function to convert high and low bytes to decimal (16-bit)
int hexToDecimal(uint8_t high, uint8_t low = 0)
{
  return (high << 8) | low;
}

// Function to parse the hex data into the Record struct
Record parseRecord(const uint8_t *data)
{
  Record record;

  // Extract values based on the data format
  record.sys = data[0];                          // Systolic pressure
  record.dia = data[1];                          // Diastolic pressure
  record.pulse = hexToDecimal(data[2], data[3]); // Heart rate
  record.heat = data[4];                         // Heat flag
  record.mean = data[5];                         // Mean pressure

  // Extract timestamp components
  int year = hexToDecimal(data[6], data[7]);
  int month = data[8];
  int day = data[9];
  int hour = data[10];
  int minute = data[11];
  int second = data[12];

  // Format timestamp into the record_time char array
  snprintf(record.record_time, sizeof(record.record_time), "%04d-%02d-%02d %02d:%02d:%02d",
           year, month, day, hour, minute, second);

  return record;
}

// Function to convert a space-separated hex string to a byte array
bool parseHexInput(const String &input, uint8_t *data, size_t maxLength, size_t &length)
{
  length = 0; // Reset length
  int start = 0;

  while (start < input.length() && length < maxLength)
  {
    int spaceIndex = input.indexOf(' ', start);
    String hexValue = input.substring(start, (spaceIndex == -1) ? input.length() : spaceIndex);

    // Convert hex string to a byte
    data[length++] = (uint8_t)strtol(hexValue.c_str(), NULL, 16);

    if (spaceIndex == -1)
      break; // No more spaces, end of string
    start = spaceIndex + 1;
  }

  return length > 0;
}