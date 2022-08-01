#include <iostream>
#include <vector>
#include "header/thread_pool.h"

#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <Packet.h>
#include <PcapFileDevice.h>

int thread_worker_func(std::string const &pcap_name)
{
	pcpp::PcapFileReaderDevice reader{"1_packet.pcap"};
	if (!reader.open())
	{
		std::cerr << "PCAP reading resolved in error!" << std::endl;
		return 1;
	}

	pcpp::RawPacket rawPacket;
	if (!reader.getNextPacket(rawPacket))
	{
		std::cerr << "Error in packet" << std::endl;
		return 1;
	}

	const pcpp::Packet & parsedPacket{&rawPacket};

	if (parsedPacket.isPacketOfType(pcpp::IPv4))
	{
		auto srcIP = parsedPacket.getLayerOfType<pcpp::IPv4Layer>()->getDstIPv4Address();
		auto dstIP = parsedPacket.getLayerOfType<pcpp::IPv4Layer>()->getDstIPv4Address();

		if (parsedPacket.isPacketOfType(pcpp::TCP))
		{
			auto sport = parsedPacket.getLayerOfType<pcpp::TcpLayer>()->getSrcPort();
			auto dport = parsedPacket.getLayerOfType<pcpp::TcpLayer>()->getDstPort();
			std::cout << "Source IP is: " << srcIP << " and Destination IP is: " << dstIP << " " << sport << " " << dport << std::endl;
		}
		
		std::cout << "Source IP is: " << srcIP << " and Destination IP is: " << dstIP << std::endl;		
	}
	reader.close();
}

int main(int argc, char* argv[])
{
	thread_pool pool{4};
	std::vector<std::future<int>> futures;
	
	for (const std::string &name : {"generated_example1.pcapng", "generated_example2.pcapng", "generated_example3.pcapng", "generated_example4.pcapng"})
	{
		futures.push_back(pool.execute(thread_worker_func, name));
	}

	for (auto &fut : futures)
	{
		std::cout << fut.get() << std::endl;
	}

	return 0;
}
