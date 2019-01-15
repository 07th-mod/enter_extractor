#include "Txa.hpp"

#include "HeaderStructs.hpp"

#include <boost/locale.hpp>

const std::locale cp932 = boost::locale::generator().generate("ja_JP.cp932");

template<typename Header>
static std::istream& readFromIStream(std::istream& stream, Header &header) {
	const size_t baseHeaderSize = sizeof(header) - sizeof(std::string);
	stream.read((char *)&header, baseHeaderSize);
	const size_t stringLength = header.headerLength - baseHeaderSize;
	char buffer[stringLength + 1];
	buffer[stringLength] = 0;
	stream.read(buffer, stringLength);
	header.name = boost::locale::conv::to_utf<char>(buffer, cp932);
	return stream;
}

std::istream& operator>> (std::istream& stream, TxaChunkPS3 &header) {
	return readFromIStream(stream, header);
}

std::istream& operator>> (std::istream& stream, TxaChunkSwitch &header) {
	return readFromIStream(stream, header);
}
