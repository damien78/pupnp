/* test_gena_subscribe_dos.c
 *
 * Regression test for issue #435:
 * Sending SUBSCRIBE requests with a large Callback header exhausts memory.
 *
 * Root cause: create_url_list() in gena_device.c calls malloc(header_len + 1)
 * with no upper bound.  With MaxSubscriptions == UPNP_INFINITE (the default),
 * subscriptions accumulate for up to 1800 s, so memory grows without limit.
 *
 * Fix: reject Callback headers longer than
 * MAX_SUBSCRIPTION_CALLBACK_HEADER_SIZE with HTTP 412 Precondition Failed
 * before calling create_url_list().
 *
 * Test logic:
 *   Send one SUBSCRIBE request whose Callback header is 0x10000 bytes long
 *   (the same order of magnitude used in the original PoC).
 *
 *   Before fix: server returns HTTP 200 (subscription accepted) → FAIL
 *   After  fix: server returns HTTP 412 (header rejected)      → PASS
 *
 * The Callback URL uses the server's own IP address so that
 * gena_validate_delivery_urls() passes the subnet check.
 */

#include "posix_overwrites.h" /* IWYU pragma: keep */
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

/* Path that the registered service listens on for SUBSCRIBE requests. */
#define EVENT_URL_PATH "/event/dos435"

/* Size of the oversized Callback URL path — matches the PoC in issue #435. */
#define LARGE_CB_PATH_LEN 0x10000u /* 65536 bytes */

/* Minimal device description with a single evented service. */
static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>dos435</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:dos435-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control/dos435</controlURL>"
	"<eventSubURL>" EVENT_URL_PATH "</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

#ifndef _WIN32
/*
 * Send one SUBSCRIBE request with a LARGE_CB_PATH_LEN-byte Callback path.
 * The Callback host is set to server_ip so the subnet check passes.
 * Returns the HTTP status code from the response, or -1 on error.
 */
static int send_subscribe_large_callback(
	const char *server_ip, unsigned short server_port)
{
	int fd = -1;
	int status = -1;
	char *req = NULL;
	size_t cap, off;
	struct sockaddr_in addr;
	char resp[512];
	struct timeval tv;
	ssize_t n;

	/* Fixed parts of the request. */
	const char *host_hdr_fmt =
		"SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"NT: upnp:event\r\n"
		"Timeout: Second-1800\r\n"
		"Connection: close\r\n"
		"Callback: <http://%s:1/"; /* path 'a'*N follows */

	/* Allocate enough space for all pieces. */
	cap = 256 + LARGE_CB_PATH_LEN + 8;
	req = malloc(cap);
	if (!req) {
		perror("malloc");
		return -1;
	}

	off = (size_t)snprintf(req,
		cap,
		host_hdr_fmt,
		server_ip,
		(unsigned)server_port,
		server_ip);
	memset(req + off, 'a', LARGE_CB_PATH_LEN);
	off += LARGE_CB_PATH_LEN;
	off += (size_t)snprintf(req + off, cap - off, ">\r\n\r\n");

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		goto done;
	}

	/* 5-second receive timeout so the test does not hang. */
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton(%s) failed\n", server_ip);
		goto done;
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("connect");
		goto done;
	}
	if (send(fd, req, off, 0) < 0) {
		perror("send");
		goto done;
	}

	n = recv(fd, resp, sizeof(resp) - 1, 0);
	if (n > 0) {
		resp[n] = '\0';
		if (sscanf(resp, "HTTP/1.%*d %d", &status) != 1)
			status = -1;
	}

done:
	if (fd >= 0)
		close(fd);
	free(req);
	return status;
}
#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: raw POSIX socket test not supported on Windows.");
	return EXIT_SUCCESS;
#else
	int rc;
	UpnpDevice_Handle handle = -1;
	const char *server_ip;
	unsigned short server_port;
	int http_status;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping test (no network?)\n",
			rc);
		/* Treat as a skip, not a hard failure, so CI passes on
		 * machines without a routable interface. */
		return EXIT_SUCCESS;
	}

	server_ip = UpnpGetServerIpAddress();
	server_port = UpnpGetServerPort();
	if (!server_ip || !server_port) {
		fprintf(stderr, "Could not determine server address\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}
	printf("Server: %s:%u\n", server_ip, server_port);

	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		DEVICE_DESC,
		sizeof(DEVICE_DESC) - 1,
		1, /* config_baseURL */
		device_callback,
		NULL,
		&handle);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpRegisterRootDevice2 failed: %d\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	http_status = send_subscribe_large_callback(server_ip, server_port);
	printf("SUBSCRIBE with %u-byte Callback path → HTTP %d\n",
		LARGE_CB_PATH_LEN,
		http_status);

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();

	if (http_status == 412) {
		puts("PASS: server rejected oversized Callback header (HTTP "
		     "412).");
		return EXIT_SUCCESS;
	}

	fprintf(stderr,
		"FAIL: expected HTTP 412 Precondition Failed, got HTTP %d.\n"
		"      HTTP 200 means the subscription was accepted with a\n"
		"      %u-byte allocation per subscription -- issue #435.\n",
		http_status,
		LARGE_CB_PATH_LEN);
	return EXIT_FAILURE;
#endif /* !_WIN32 */
}
