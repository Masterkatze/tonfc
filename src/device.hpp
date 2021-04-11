#include <nfc/nfc.h>

#include <array>
#include <vector>

class Device
{
public:
	void open();
	std::vector<uint8_t> receive();
	void send(std::vector<uint8_t> data);

private:
	nfc_device *device;

	uint8_t saved_data[8];
};
