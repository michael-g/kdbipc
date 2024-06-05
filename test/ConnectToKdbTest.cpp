#include <gtest/gtest.h>
#include <memory>

#include "KdbType.h"

#include "../src/KdbType.cpp"

using namespace mg7x;

namespace mg7x::test {

static constexpr short MG_PORT = 30098;

static int sk_open(int port);
static void sk_write(int fd, const void *buf, size_t count, ssize_t & wrt);
static void sk_read(int fd, std::unique_ptr<int8_t> & src, ssize_t & red);
static void sk_msg_read(int fd, std::unique_ptr<int8_t> & ptr, ssize_t & red);
static int sk_close(int fd);

static void hopen(int & fd, const char *usr)
{
	fd = sk_open(MG_PORT);
	if (fd <= 0)
		return;

	std::unique_ptr<char> msg{};

	size_t msg_len = KdbUtil::writeLoginMsg(usr, "", msg);

	EXPECT_NE(0, msg_len) << "ERROR: failed to allocate memory for the login bytes";
	if (0 == msg_len) {
		fd = sk_close(fd);
		return;
	}

	ssize_t wr_len, rd_len;
	sk_write(fd, msg.get(), msg_len, wr_len);

	EXPECT_EQ(msg_len, wr_len) << "ERROR: failed to write msg_len bytes to socket";
	if (msg_len != wr_len) {
		fd = sk_close(fd);
		return;
	}

	std::unique_ptr<int8_t> src{};
	sk_read(fd, src, rd_len);

	EXPECT_EQ(1, rd_len);
	if (1 != rd_len) {
		fd = sk_close(fd);
		return;
	}
}

static void open_and_write(char *mem, size_t len, const char *usr)
{
	int fd;
	hopen(fd, usr);
	if (-1 == fd) 
		FAIL() << "Could not open a connection to q";
	
	ssize_t err;
	sk_write(fd, mem, len, err);
	EXPECT_EQ(len, err);
	fd = sk_close(fd);
}

template<typename T>
static std::unique_ptr<T> open_write_and_read(const char *usr, char *src, size_t len)
{
	int fd;
	hopen(fd, usr);
	if (-1 == fd) { 
		std::cerr << "Could not open a connection to q at line " << (__LINE__ - 2) << std::endl;
		return {};
	}
	
	ssize_t err;
	sk_write(fd, src, len, err);
	EXPECT_EQ(len, err);

	ssize_t red;
	std::unique_ptr<int8_t> ipc{};
	sk_msg_read(fd, ipc, red);
	if (-1 == red) {
		std::cerr << "Failure in sk_msg_read at line " << (__LINE__ - 2) << std::endl;
		return {};
	} 

	ReadMsgResult rmr;
	KdbIpcMessageReader rdr{};
	if (!rdr.readMsg(ipc.get(), len, rmr)) {
		std::cerr << "Message reported incomplete by KdbIpcMessageReader at line " << (__LINE__ - 1) << std::endl;
		return {};
	}

	KdbBase *obj = rmr.message.release();

	fd = sk_close(fd);

	return std::unique_ptr<T>{reinterpret_cast<T*>(obj)};
}

TEST(KdbConnectedTest, TestKdbConnect)
{
	int fd;
	hopen(fd, "Basic connection test");
	fd = sk_close(fd);
}

TEST(KdbConnectedTest, TestSimpleWrite)
{
	auto unq = std::make_unique<KdbCharVector>(std::string_view{"-1\"Hello, world!\""});
	auto shr = std::shared_ptr<KdbCharVector>(unq.release());
	KdbIpcMessageWriter writer{KdbMsgType::ASYNC, shr};
	size_t rqd = writer.bytesRemaining();

	auto mem = std::make_unique<char[]>(rqd);
	WriteResult wr = writer.write(mem.get(), rqd);
	EXPECT_EQ(WriteResult::WR_OK, wr);
	// q)-8!"-1\"Hello, world!\""
	// 0x010000001f0000000a00110000002d312248656c6c6f2c20776f726c642122
	open_and_write(mem.get(), rqd, "Say 'hello, world!'");
}

TEST(KdbConnectedTest, TestTableWriter)
{
	const std::vector<std::string_view> cols{ "time", "sym", "price", "size"};
	std::unique_ptr<KdbTable> tbl = std::make_unique<KdbTable>("tsfj", cols);
	if (!tbl)
		FAIL() << "Failed in KdbTable::ctor";
	
}

// TEST(KdbConnectedTest, TestKdbUpdMessage1)
// {
// 	std::unique_ptr<KdbUpdMessage> msg = KdbUpdMessage::build(".u.upd", "trade", "psfj", 32);
// 	if (!msg)
// 		FAIL() << "Failure in KdbUpdMessage::build";
	
// 	KdbTimestampVector* time = msg->getCol<KdbTimestampVector>(0);
// 	KdbSymbolVector* sym = msg->getCol<KdbSymbolVector>(1);
// 	KdbFloatVector* price = msg->getCol<KdbFloatVector>(2);
// 	KdbLongVector* size = msg->getCol<KdbLongVector>(3);

// 	const size_t COL_COUNT = 4;
// 	const size_t ROW_COUNT = 8;

// 	for (size_t i = 0 ; i < ROW_COUNT ; i++) {
// 		// use "n" as the column-type specifier to use a KdbTimespanVector instead
// 		//time->append(time::Timespan::now(chr::current_zone()));
// 		time->append(time::Timestamp::now());
// 		sym->append("VOD.L");
// 		price->append(185.0 + i);
// 		size->append(42 + i);
// 	}
// 	// Which writes something like this, if you define .u.upd properly:
// 	// time                          sym   price size
// 	// ----------------------------------------------
// 	// 2024.02.08D20:20:50.425890139 VOD.L 185   42  
// 	// 2024.02.08D20:20:50.425891083 VOD.L 186   43  
// 	// 2024.02.08D20:20:50.425891506 VOD.L 187   44  
// 	// 2024.02.08D20:20:50.425891923 VOD.L 188   45  
// 	// 2024.02.08D20:20:50.425892339 VOD.L 189   46  
// 	// 2024.02.08D20:20:50.425892749 VOD.L 190   47  
// 	// 2024.02.08D20:20:50.425893160 VOD.L 191   48  
// 	// 2024.02.08D20:20:50.425893569 VOD.L 192   49
// 	auto lda = [](KdbUpdMessage *msg) {
// 		const uint64_t cap = msg->wireSize();
// 		auto mem = std::make_unique<char[]>(cap);
// 		WriteBuf buf{mem.get(), cap};
// 		EXPECT_EQ(WriteResult::WR_OK, msg->write(buf));

// 		return open_write_and_read<KdbList>("TestKdbUpdMessage1", mem.get(), static_cast<size_t>(cap));
// 	};
// 	std::unique_ptr<KdbList> rtn = lda(msg.get());
// 	if (!rtn)
// 		FAIL() << "Message should be complete";

// 	// Check the structure of the outer message: expect a three-element list
// 	EXPECT_EQ(3, rtn->count());

// 	// Check the first element has type SYMBOL_ATOM and value ".u.upd"
// 	EXPECT_TRUE(rtn->getIdx(0).has_value());
// 	KdbObj *obj = rtn->getIdx(0).value();
// 	EXPECT_EQ(KdbType::SYMBOL_ATOM, obj->type());
// 	EXPECT_EQ(".u.upd", dynamic_cast<KdbSymbolAtom*>(obj)->asView());

// 	// Check the second element is a symbol atom with value "trade"
// 	EXPECT_TRUE(rtn->getIdx(1).has_value());
// 	obj = rtn->getIdx(1).value();
// 	EXPECT_EQ(KdbType::SYMBOL_ATOM, obj->type());
// 	EXPECT_EQ("trade", dynamic_cast<KdbSymbolAtom*>(obj)->asView());

// 	// Check the third element is a list
// 	EXPECT_TRUE(rtn->getIdx(2).has_value());
// 	obj = rtn->getIdx(2).value();
// 	EXPECT_EQ(KdbType::LIST, obj->type());
// 	KdbList *vals = dynamic_cast<KdbList*>(obj);

// 	// Check the list content: expect four elements with the given types
// 	EXPECT_EQ(COL_COUNT, vals->count());
// 	size_t i = 0;
// 	for (auto typ : std::array<KdbType, COL_COUNT>{ KdbType::TIMESTAMP_VECTOR, KdbType::SYMBOL_VECTOR, KdbType::FLOAT_VECTOR, KdbType::LONG_VECTOR }) {
// 		EXPECT_EQ(typ, vals->getIdx(i++).value()->type());
// 	}

// 	EXPECT_EQ(ROW_COUNT, dynamic_cast<KdbSequence*>(vals->getIdx(0).value())->count());

// 	// Check the list content matches the input data
// 	for (size_t i = 0 ; i < ROW_COUNT ; i++) {
// 		EXPECT_EQ(time->getIdx(i), dynamic_cast<KdbTimestampVector*>(vals->getIdx(0).value())->getIdx(i));
// 		EXPECT_EQ(sym->getIdx(i), dynamic_cast<KdbSymbolVector*>(vals->getIdx(1).value())->getIdx(i));
// 		// NB we're comparing floating-point values here, seems to work without specifying an epsilon
// 		EXPECT_EQ(price->getIdx(i), dynamic_cast<KdbFloatVector*>(vals->getIdx(2).value())->getIdx(i));
// 		EXPECT_EQ(size->getIdx(i), dynamic_cast<KdbLongVector*>(vals->getIdx(3).value())->getIdx(i));
// 	}

// 	msg->clear();

// 	for (size_t i = 0 ; i < ROW_COUNT ; i++) {
// 		// use "n" as the column-type specifier to use a KdbTimespanVector instead
// 		//time->append(time::Timespan::now(chr::current_zone()));
// 		time->append(time::Timestamp::now());
// 		sym->append("AZN.L");
// 		price->append(330.0 + i);
// 		size->append(61 + i);
// 	}

// 	// destroy the previous IPC message we've read, explictly
// 	rtn.reset();

// 	rtn = lda(msg.get());
// 	if (!rtn)
// 		FAIL() << "Message should be complete";

//   // Let 'vals' point to the new list in the IPC response
// 	vals = dynamic_cast<KdbList*>(rtn->getIdx(2).value());
// 	EXPECT_EQ(ROW_COUNT, dynamic_cast<KdbTimestampVector*>(vals->getIdx(0).value())->count());

// 	// Check the list content matches the input data
// 	for (size_t i = 0 ; i < ROW_COUNT ; i++) {
// 		EXPECT_EQ(time->getIdx(i), dynamic_cast<KdbTimestampVector*>(vals->getIdx(0).value())->getIdx(i));
// 		EXPECT_EQ(sym->getIdx(i), dynamic_cast<KdbSymbolVector*>(vals->getIdx(1).value())->getIdx(i));
// 		EXPECT_EQ(price->getIdx(i), dynamic_cast<KdbFloatVector*>(vals->getIdx(2).value())->getIdx(i));
// 		EXPECT_EQ(size->getIdx(i), dynamic_cast<KdbLongVector*>(vals->getIdx(3).value())->getIdx(i));
// 	}
// }

} // end namespace mg7x::test

// lifted from https://www.linuxhowtos.org/C_C++/socket.htm
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


namespace mg7x::test {

static int sk_close(int fd)
{
	close(fd);
	return -1;
}

static void sk_read(int fd, std::unique_ptr<int8_t> & src, ssize_t & red)
{
	static constexpr size_t cap = 64 * 1024;
	void *dst = malloc(cap);
	if (nullptr == dst) {
		FAIL() << "ERROR: failed to allocate " << cap << " bytes";
	}

	for (int i = 0 ; i < 3 ; i++) {
		red = read(fd, dst, cap);
		if (red < 0) {
			if (EINTR == red)
				continue;
			FAIL() << "ERROR: failed in 'read' on FD " << fd << "; error was: " << strerror(errno);
		}
		src.reset(static_cast<int8_t*>(dst));
		return;
	}
	
}

static void sk_msg_read(int fd, std::unique_ptr<int8_t> & ptr, ssize_t & red)
{
	char hdr[SZ_MSG_HDR] = {0};
	size_t msg_len;
	size_t acc = 0;
	do {
		ssize_t err = read(fd, hdr, SZ_MSG_HDR);
		if (-1 == err) {
			if (EAGAIN == errno)
				continue;
			red = err;
			FAIL() << "while reading message header: " << strerror(errno);
			return;
		}
		acc += err;
	} while (acc < SZ_MSG_HDR);

	msg_len = reinterpret_cast<int32_t*>(hdr)[1];

	int8_t *msg = new int8_t[msg_len];
	memcpy(msg, hdr, SZ_MSG_HDR);
	
	do {
		ssize_t err = read(fd, msg + acc, msg_len - acc);
		if (-1 == err) {
			if (EAGAIN == err)
				continue;
			delete[] msg;
			red = err;
			FAIL() << "while reading message body: " << strerror(errno);
			return;
		}
		acc += err;
	} while (acc < msg_len);

	ptr.reset(msg);
	red = msg_len;
}

static void sk_write(int fd, const void *buf, size_t count, ssize_t & wrt)
{
	for (int i = 0 ; i < 3 ; i++) {
		wrt = write(fd, buf, count);
		if (wrt < 0) {
			if (EINTR == wrt)
				continue;
			FAIL() << "ERROR: failed to write " << count << " bytes on FD " << fd << "; error is " << strerror(errno);
		}
		return;
	}
}

static int sk_open(int port)
{
		int sockfd, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;

		char buffer[256];
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		EXPECT_LT(0 , sockfd) << "Error opening socket"; 
		server = gethostbyname("localhost");
		EXPECT_NE(nullptr, server) << "ERROR, no such host";

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(port);
		int err = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
		EXPECT_EQ(0, err) << "ERROR connecting";
		return sockfd;
}
}


int main(int argc, char **argv)
{
	using namespace mg7x::test;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}