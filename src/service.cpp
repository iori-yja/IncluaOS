// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath> // rand()
#include <sstream>

#include <os>
#include <net/inet4>
#include <timers>
#include <unistd.h>
#include <sys/socket.h>
#include <net/http/request.hpp>
#include <net/http/response.hpp>
#include <sys/times.h>
#include <term>

std::unique_ptr<Terminal> term;
net::tcp::Connection_ptr client_conn;

char *eliminate_command(char *buf, int n)
{
	char *ret = (char *) malloc(1024);
	int copied_len = 0;

	while(n) {
		switch (*buf) {
			case 240:
				break;
			case 255:
				/* Interpret as Command */
				buf ++;
				if (*buf >= 251) {
					buf ++;
				}
			default:
				if (*buf < 0x20)
					break;
				*(ret + copied_len) = *buf;
				copied_len ++;
				break;
		}
		buf ++;
		n --;
	}
	*(ret + copied_len) = 0;

	return ret;
}

extern "C" {
	extern int lua_main (int, char **);
}

using namespace std::chrono;

class Lua {
	public:
	int run() {
		char *progname = (char *) "lua";
		char **argv;
		argv = (char **) malloc(1 * sizeof(char *));
		*argv = progname;

		lua_main(1, argv);
		return 0;
	}
};

extern "C" {
	void write_to_telnet (char *string) {
		term->write(string);
	}
	char *read_from_telnet (char *s, int l) {
		client_conn->on_read(1024, [s] (auto buf, size_t n) {
			char *test = (char *)buf.get();
			char *raw = eliminate_command(test, n);
			strcpy(s, raw);
			printf("Incomming: %s::", s);
			int i = 0;
			while(*(s + i) != 0) {
				printf("%d", *(s+i));
				i++;
			}
			printf("\n");
			free(raw);
		});
		OS::block();
		return s;
	}
}

/*
void posix_sockets(inet& test)
{
	int fd socket(AF_INET, SOCK_DGRAM, 0);
	struct sockadder_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AFINET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(23);
	int res = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
	fd = dup2(fd, stdin);
}*/

void Service::start(const std::string&)
{
  // Static IP configuration will get overwritten by DHCP, if found
  auto& inet = net::Inet4::ifconfig<0>(10);
  inet.network_config({ 10,0,0,42 },      // IP
                      { 255,255,255,0 },  // Netmask
                      { 10,0,0,1 },       // Gateway
                      { 8,8,8,8 });       // DNS

  // Set up a TCP server on port 80
  auto& server = inet.tcp().bind(23);

  printf("Server listening: %s \n", server.local().to_string().c_str());

	server.on_connect(
		[&inet] (auto client) {
			printf("Connected [Client]: %s\n", client->to_string().c_str());
			term = std::make_unique<Terminal> (client);
			term->add("echo", "echo", [client] (const std::vector<std::string>&) -> int {
					client->on_read(1024, [] (auto buf, size_t n) {
							char *test = (char *)buf.get();
							char *raw = eliminate_command(test, n);
							term->write(raw);
							free(raw);
					});
					return 0;
			});
			term->add("lua", "lua", [client] (const std::vector<std::string>&) -> int {
					term->write("lua...\n");
					client_conn = client;
					Lua runtime;
					runtime.run();
					return 0;
			});
			term->add("halt", "halt", [] (const std::vector<std::string>&) -> int {
					OS::halt();
					return 0;
			});
	});
	INFO("TERM", "Connect to terminal with $ telnet %s ", inet.ip_addr().str().c_str());

}
