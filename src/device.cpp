#include "device.hpp"

#include <iostream>

void Device::open()
{
	// TODO: check if nfc is connected
	// nfc_initiator_target_is_present?

	std::array<uint8_t, 6> query = {6, 0, 254, 225, 0, 0}; // {6, 0, -2, -31, 0, 0}
	std::array<uint8_t, 8> expected_response = {0, 224, 0, 0, 0, 255, 255, 0}; // {0, -32, 0, 0, 0, -1, -1, 0}
	std::array<uint8_t, 4> unknown_array = {3, 254, 0, 26}; // {3, -2, 0, Ascii.SUB};

	uint8_t response[256];

	auto bytes_count = nfc_initiator_transceive_bytes(device, query.data(), query.size(), response, sizeof(response), -1);
	if(bytes_count < 0)
	{
		auto error_code = &bytes_count;
		std::cerr << "Failed to send open command, error code: " << error_code << std::endl;
		return;
	}

	if(response[0] != bytes_count || bytes_count != 18)
	{
		std::cerr << "Invalid response lenght" << std::endl;
		return;
	}

	if(response[1] != 1)
	{
		std::cerr << "Invalid response code " << std::hex << response[1] << std::endl;
		return;
	}

	uint8_t bArr2[8];
	std::copy(response + 2, response + 2 + sizeof(bArr2), bArr2);

	std::copy(bArr2, bArr2 + sizeof(saved_data), saved_data);

	//std::copy(response + 2, response + 2 + sizeof(saved_data), saved_data);

	uint8_t bArr3[8];
	std::copy(response + 2 + sizeof(bArr2), response + 2 + sizeof(bArr2) + sizeof(bArr3), bArr3);

	for(auto i = 2u; i < expected_response.size(); i++)
	{
		if(bArr3[i] != expected_response[i - 2])
		{
			std::cerr << "Invalid response pmm (whatever that means)" << std::endl;
			return;
		}
	}

	for(auto i = 0u; i < unknown_array.size(); i++)
	{
		if(bArr2[i] != unknown_array[i])
		{
			std::cerr << "IDm failed to match (polling)" << std::endl;
			return;
		}
	}
}

std::vector<uint8_t> Device::receive()
{
	// TODO: check if nfc is connected

	std::array<uint8_t, 14> query_base = {0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 1, 9, 0, 0};

	uint8_t query[38]; // query_base.size() + 24
	std::copy(query_base.data(), query_base.data() + sizeof(query_base), query);

	query[0] = 38;

	std::copy(saved_data, saved_data + sizeof(saved_data), query + 2);

	query[13] = 12;

	for(int i = 0; i < 12; i++)
	{
		uint8_t bArr2[2] = {128, 0}; // {UnsignedBytes.MAX_POWER_OF_TWO, 0}
		bArr2[1] = (i | 176) & 255;
		std::copy(bArr2, bArr2 + sizeof(bArr2), query + (i * 2) + 14);
	}

	uint8_t response[256];

	auto bytes_count = nfc_initiator_transceive_bytes(device, query, sizeof(query), response, sizeof(response), -1);
	if(bytes_count < 0)
	{
		auto& error_code = bytes_count;
		throw std::runtime_error("Failed to send receive command, error code: " + std::to_string(error_code));
	}

	if(response[0] != bytes_count)
	{
		throw std::runtime_error("Invalid response lenght");
	}

	if(response[1] != 7)
	{
		throw std::runtime_error("Invalid response code " + std::to_string(response[1]));
	}

	for(auto i = 0u; i < sizeof(saved_data); i++)
	{
		if(response[i + 2] != saved_data[i])
		{
			throw std::runtime_error("Invalid response idm (whatever that means)");
		}
	}

	if(response[10] == 255)
	{
		if(response[11] == 161)
		{
			throw std::runtime_error("Invalid service num");
		}
		else if(response[11] == 162)
		{
			throw std::runtime_error("Invalid block num");
		}
		else
		{
			throw std::runtime_error("Unexpected status flag " + std::to_string(response[11]));
		}
	}

	if(response[12] < 1 || response[12] > 12)
	{
		throw std::runtime_error("Invalid block num");
	}

	const auto& payload_lenght = response[13];

	std::vector<uint8_t> data;
	data.reserve(payload_lenght);

	for(auto i = 13u; i < payload_lenght; i++)
	{
		data.emplace_back(response[i]);
	}

	return data;
}

void Device::send(std::vector<uint8_t> data)
{
	if(data.empty())
	{
		throw std::runtime_error("Empty data");
	}

	int block_count = data.size() / 16;
	if(data.size() % 16 > 0)
	{
		block_count++;
	}

	std::vector<std::vector<uint8_t>> block_headers;
	block_headers.reserve(block_count);

	int i2 = 0;
	for(int i3 = 0; i3 < block_count; i3++)
	{
		std::vector<uint8_t> block_header(2);
		if(i3 <= 255)
		{
			block_header = {128, 0}; // {UnsignedBytes.MAX_POWER_OF_TWO, 0}
			block_header.at(1) = (i3 | 176) & 255;
			i2++;
		}
		else
		{
			block_header = {0, 0, 0};
			block_header.at(1) = (i3 | 176) & 255;
			block_header.at(2) = ((i3 | 176) >> 8) & 255;
		}
		block_headers.emplace_back(std::move(block_header));
	}

	std::vector<std::vector<uint8_t>> block_data;
	block_data.reserve(block_count);

	for(int i4 = 0; i4 < block_count; i4++)
	{
		std::vector<uint8_t> block_datum;
		block_datum.reserve(16);

		auto size = (data.size() < (i4 + 1) * 16u) ? (data.size() % 16) : data.size();

		for(auto j = i4 * 16u; j < size; j++)
		{
			block_datum.push_back(data.at(j));
		}

		block_data.emplace_back(std::move(block_datum));
	}

	std::array<uint8_t, 14> query_base = {0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 1, 9, 0, 0};

	//0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
	//0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 1, 9, 0, 0
	//0 input data len
	//   1 command id
	//      2-9 saved_data
	//                             10 ?
	//                                11 ?
	//                                   12 ?
	//                                      13 number of blocks (block size 16)
	//                                         14 - x data in vector (from bArr5)
	//                                            15 - x data in vector2 (from bArr6)

	std::vector<uint8_t> query(query_base.size() + ((block_count - i2) * 3) + (i2 * 2) + (block_data.size() * 16));

	for(auto i = 0u; i < query_base.size(); i++)
	{
		query.at(i) = query_base.at(i);
	}

	query.at(0) = query.size();

	for(auto i = 0u; i < sizeof(saved_data); i++)
	{
		query.at(i + 2) = saved_data[i];
	}

	query.at(13) = block_count;

	int query_offset = 14;
	for(auto i = 0u; i < block_headers.size(); i++)
	{
		const auto& block_header = block_headers.at(i);

		for(auto j = 0u; j < block_header.size(); j++)
		{
			query.at(query_offset + j) = block_header.at(j);
		}

		query_offset += block_header.size();
	}

	for(auto i = 0u; i < block_data.size(); i++)
	{
		const auto& block_datum = block_data.at(i);

		for(auto j = 0u; j < block_datum.size(); j++)
		{
			query.at(query_offset + j) = block_datum.at(j);
		}

		query_offset += sizeof(block_datum);
	}

	uint8_t response[256];

	auto bytes_count = nfc_initiator_transceive_bytes(device, query.data(), query.size(), response, sizeof(response), -1);
	if(bytes_count < 0)
	{
		auto& error_code = bytes_count;
		throw std::runtime_error("Failed to send receive command, error code: " + std::to_string(error_code));
	}

	if(response[0] != bytes_count || bytes_count != 12)
	{
		throw std::runtime_error("Invalid response lenght");
	}

	if(response[1] != 9)
	{
		throw std::runtime_error("Invalid response code " + std::to_string(response[1]));
	}

	for(auto i = 0u; i < sizeof(saved_data); i++)
	{
		if(response[i + 2] != saved_data[i])
		{
			throw std::runtime_error("Invalid response idm (whatever that means)");
		}
	}

	if(response[10] != 255)
	{
		if(response[11] == 161)
		{
			throw std::runtime_error("Invalid service num");
		}
		else if(response[11] == 162)
		{
			throw std::runtime_error("Invalid block num");
		}
		else
		{
			throw std::runtime_error("Unexpected status flag " + std::to_string(response[11]));
		}
	}
}