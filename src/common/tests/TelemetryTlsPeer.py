# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""Loopback-only TLS peer for owner-run tests; generates temporary test keys."""

import os
import re
import signal
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import time


def certificate(openssl, directory, name, subject, ecdsa=False):
    key = ["-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256"] if ecdsa else ["-newkey", "rsa:2048"]
    subprocess.check_call(
        [
            openssl, "req", "-new", "-x509",
        ] + key + [
            "-nodes",
            "-days", "1", "-sha256", "-subj", "/CN=" + subject,
            "-config", os.path.join(directory, "openssl.cnf"),
            "-keyout", os.path.join(directory, name + "-key.pem"),
            "-out", os.path.join(directory, name + ".pem"),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        timeout=10,
    )


def expired_certificate(openssl, directory, subject, san):
    certificate(openssl, directory, "root", "Expiry Test Root")
    root = os.path.join(directory, "root.pem")
    server = os.path.join(directory, "server.pem")
    with tempfile.TemporaryDirectory(dir=directory) as authority:
        config = os.path.join(authority, "ca.cnf")
        request = os.path.join(authority, "server.csr")
        with open(os.path.join(authority, "index"), "w"):
            pass
        with open(os.path.join(authority, "serial"), "w") as serial:
            serial.write("01\n")
        with open(config, "w") as output:
            output.write(
                "[ca]\ndefault_ca=issuer\n[issuer]\n"
                "database=" + os.path.join(authority, "index") + "\n"
                "serial=" + os.path.join(authority, "serial") + "\n"
                "new_certs_dir=" + authority + "\n"
                "default_md=sha256\ndefault_days=1\npolicy=policy\nunique_subject=no\n"
                "[policy]\ncommonName=supplied\n"
                "[server]\nbasicConstraints=critical,CA:FALSE\n"
                "keyUsage=critical,digitalSignature,keyEncipherment\n"
                "extendedKeyUsage=serverAuth\nsubjectAltName=" + san + "\n"
            )
        subprocess.check_call(
            [openssl, "req", "-new", "-key", os.path.join(directory, "server-key.pem"),
             "-subj", "/CN=" + subject, "-config", os.path.join(directory, "openssl.cnf"),
             "-out", request],
            stdout=subprocess.DEVNULL, timeout=10)
        sign = [openssl, "ca", "-batch", "-notext", "-config", config,
                "-cert", root, "-keyfile", os.path.join(directory, "root-key.pem"),
                "-in", request, "-extensions", "server"]
        valid = os.path.join(authority, "valid.pem")
        verify = [openssl, "verify", "-CAfile", root, "-purpose", "sslserver",
                  "-verify_hostname", subject]
        # Validate the same CSR/issuer/extensions without requiring -no_check_time
        # (absent from OpenSSL 1.0.2), then issue its expired counterpart.
        subprocess.check_call(sign + ["-out", valid],
                              stdout=subprocess.DEVNULL, timeout=10)
        subprocess.check_call(verify + [valid],
                              stdout=subprocess.DEVNULL, timeout=10)
        subprocess.check_call(
            sign + ["-out", server, "-startdate", "20000101000000Z",
                    "-enddate", "20000102000000Z"],
            stdout=subprocess.DEVNULL, timeout=10)

    try:
        subprocess.check_output(verify + [server], stderr=subprocess.STDOUT, timeout=10)
    except subprocess.CalledProcessError as error:
        if not re.search(rb"error 10 at 0 depth lookup:\s*certificate has expired", error.output):
            raise RuntimeError("Unexpected certificate verification failure: " +
                               error.output.decode("utf-8", errors="replace")) from error
    else:
        raise RuntimeError("Expired certificate fixture unexpectedly verified")


def receive(connection, count):
    result = bytearray()
    while len(result) < count:
        part = connection.recv(min(16384, count - len(result)))
        if not part:
            raise RuntimeError("Client disconnected before completing its request")
        result.extend(part)
    return bytes(result)


ARIA_HOST = "mobile.events.data.microsoft.com"


def headers(connection):
    wire = bytearray()
    while not wire.endswith(b"\r\n\r\n"):
        wire.extend(receive(connection, 1))
        if len(wire) > 8192:
            raise RuntimeError("Oversized request headers")
    lines = bytes(wire).decode("ascii").split("\r\n")
    fields = {}
    for line in lines[1:-2]:
        name, value = line.split(":", 1)
        name = name.lower()
        if name in fields:
            raise RuntimeError("Duplicate request header")
        fields[name] = value.strip()
    return lines[0], fields


def aria_peer(listener, context, mode, seen_names):
    connections = 2 if mode in ("aria-close", "aria-recover", "aria-worker-recover") else 1
    event_bodies = set()
    run_id = None
    for connection_index in range(connections):
        connection, _ = listener.accept()
        with connection:
            connection.settimeout(10)
            first, fields = headers(connection)
            if (first != "CONNECT " + ARIA_HOST + ":443 HTTP/1.1" or
                    fields.get("host") != ARIA_HOST + ":443" or
                    "apikey" in fields or "content-length" in fields):
                raise RuntimeError("Invalid CONNECT or exposed collector credentials")
            authorization = fields.get("proxy-authorization")
            if mode == "aria-auth":
                if authorization != "Basic dXNlcjpwQHNz":
                    raise RuntimeError("Missing or invalid proxy credentials")
            elif authorization is not None:
                raise RuntimeError("Unexpected proxy credentials")
            if mode == "aria-proxy-timeout":
                time.sleep(3)
                return
            replies = {
                "aria-407": b"HTTP/1.1 407 Proxy Authentication Required\r\n\r\n",
                "aria-redirect": b"HTTP/1.1 302 Redirect\r\nLocation: https://untrusted.invalid/\r\n\r\n",
                "aria-bad-connect": b"HTTP/1.1 200 OK\r\n Invalid: header\r\n\r\n",
                "aria-long-connect": b"HTTP/1.1 200 OK\r\nX: " + b"x" * 2048 + b"\r\n\r\n",
                "aria-truncated-connect": b"HTTP/1.1 200 OK\r\n",
                "aria-upgrade": b"HTTP/1.1 101 Switching Protocols\r\n\r\n",
                "aria-many-interim": b"HTTP/1.1 100 Continue\r\n\r\n" * 5,
            }
            if mode in replies:
                connection.sendall(replies[mode])
                return
            reply = (b"HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 Connection Established\r\n"
                     b"Content-Length: 999999\r\nTransfer-Encoding: chunked\r\n\r\n")
            for byte in reply:
                connection.sendall(bytes(bytearray([byte])))
            try:
                secured = context.wrap_socket(connection, server_side=True)
            except ssl.SSLError:
                if mode == "aria-wrong-name":
                    return
                raise
            with secured:
                if seen_names[-1:] != [ARIA_HOST]:
                    raise RuntimeError("Collector SNI was not preserved through CONNECT")
                if hasattr(secured, "selected_alpn_protocol") and secured.selected_alpn_protocol() != "http/1.1":
                    raise RuntimeError("Invalid collector ALPN")
                count = 10000 if mode == "aria-live" else 2 if mode in (
                    "aria-reuse", "aria-auth", "aria-worker", "aria-producer") else 1
                for sequence in range(count):
                    first, fields = headers(secured)
                    if (first != "POST /OneCollector/1.0/ HTTP/1.1" or
                            fields.get("host") != ARIA_HOST or
                            fields.get("apikey") != "fixture-token" or
                            fields.get("client-id") != "NO_AUTH" or
                            fields.get("accept-encoding") != "identity" or
                            fields.get("content-type") != "application/bond-compact-binary" or
                            "proxy-authorization" in fields):
                        raise RuntimeError("Invalid collector headers or leaked proxy credentials")
                    size = int(fields["content-length"])
                    if size <= 0 or size > 16384:
                        raise RuntimeError("Invalid collector request length")
                    body = receive(secured, size)
                    if mode.startswith("aria-live"):
                        for required in (b"StatusTrace", b"*** Distilled 1DS SDK test No. 66 ***", b"731001",
                                         b"CorrelationId", b"ResultString", b"o:fixture"):
                            if required not in body:
                                raise RuntimeError("Synthetic event missing required field")
                        identifiers = set(re.findall(
                            b"[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}", body))
                        if len(identifiers) != 1:
                            raise RuntimeError("Missing or inconsistent run correlation")
                        identifier = next(iter(identifiers))
                        if run_id is not None and identifier != run_id:
                            raise RuntimeError("Run correlation changed between events")
                        run_id = identifier
                        if body in event_bodies:
                            raise RuntimeError("Repeated synthetic event instead of a new sequence")
                        event_bodies.add(body)
                    elif mode.startswith("aria-worker") or mode == "aria-producer":
                        event = b"BaselineRun" if mode == "aria-producer" and sequence == 1 else b"CrashDetected"
                        for required in (event, b"OSConfig-C/0.1", b"o:fixture", b"CorrelationId",
                                         b"worker-fixture"):
                            if required not in body:
                                raise RuntimeError("Worker event missing expected schema or marker")
                        if body in event_bodies:
                            raise RuntimeError("Worker replayed an event")
                        event_bodies.add(body)
                    elif body != b"encoded-fixture":
                        raise RuntimeError("Incorrect or replayed event body")
                    if mode in ("aria-drop", "aria-live-drop"):
                        os.close(secured.detach())
                        return
                    if mode in ("aria-recover", "aria-worker-recover") and connection_index == 0:
                        os.close(secured.detach())
                        break
                    if mode in ("aria-read-timeout", "aria-worker-timeout"):
                        time.sleep(3)
                        return
                    status, extra = b"200 OK", b""
                    response = b'{"acc":1}' if mode == "aria-live" else b'{"acc":1,"rej":0}'
                    if mode in ("aria-reject", "aria-live-reject", "aria-worker-reject"):
                        response = b'{"acc":0,"rej":1}'
                    elif mode in ("aria-empty", "aria-live-empty", "aria-worker-empty"):
                        response = b""
                    elif mode in ("aria-throttle", "aria-live-throttle", "aria-worker-throttle"):
                        status, extra, response = b"429 Too Many Requests", b"Retry-After: 60\r\n", b""
                    elif mode in ("aria-kill", "aria-worker-kill", "aria-worker-bad-kill", "aria-bad-kill"):
                        extra = b"kill-tokens: fixture-token\r\nkill-duration: 60\r\n"
                        if mode in ("aria-worker-bad-kill", "aria-bad-kill"):
                            response = b'{"acc":'
                    elif mode == "aria-close":
                        extra = b"Connection: close\r\n"
                    # Clock guidance must not be mistaken for acceptance or throttle.
                    extra += b"time-delta-millis: 0\r\n"
                    secured.sendall(b"HTTP/1.1 " + status + b"\r\nContent-Length: " +
                                    str(len(response)).encode("ascii") + b"\r\n" + extra + b"\r\n" + response)
                if mode in ("aria-throttle", "aria-kill", "aria-reject", "aria-empty", "aria-bad-kill",
                            "aria-live-throttle", "aria-live-reject", "aria-live-empty",
                            "aria-worker-throttle", "aria-worker-kill", "aria-worker-bad-kill",
                            "aria-worker-reject", "aria-worker-empty"):
                    # No additional application request is permitted on this connection.
                    try:
                        if secured.recv(1):
                            raise RuntimeError("Unexpected request after suppression/rejection")
                    except (ssl.SSLError, ConnectionResetError):
                        pass


def main():
    openssl, directory, mode = sys.argv[1:]
    os.umask(0o077)
    name = ARIA_HOST if mode.startswith("aria-") else "localhost"
    if mode in ("wrong-name", "aria-wrong-name"):
        name = "wrong.example"
    if mode == "numeric-dns":
        name = "127.0.0.1"
    elif mode == "partial-wildcard":
        name = "local.example.test"
    san = "DNS:" + name
    if mode == "partial-wildcard":
        san = "DNS:loc*.example.test"
    if mode != "numeric-dns":
        san += ",IP:127.0.0.1"
    with open(os.path.join(directory, "openssl.cnf"), "w") as config:
        config.write(
            "[req]\ndistinguished_name=dn\nx509_extensions=extensions\n"
            "[dn]\n[extensions]\nbasicConstraints=critical,CA:TRUE\n"
            "keyUsage=critical,digitalSignature,keyEncipherment,keyCertSign\n"
            "extendedKeyUsage=serverAuth\nsubjectAltName=" + san + "\n"
        )
    certificate(openssl, directory, "server", name, mode == "ecdsa")
    if mode == "expired":
        expired_certificate(openssl, directory, name, san)
    elif mode == "untrusted":
        certificate(openssl, directory, "root", "Other Test Root")
    else:
        with open(os.path.join(directory, "server.pem"), "rb") as source:
            with open(os.path.join(directory, "root.pem"), "wb") as target:
                target.write(source.read())

    signal.signal(signal.SIGALRM, signal.SIG_DFL)
    signal.alarm(1200 if mode == "aria-live" else 20)
    protocol = getattr(ssl, "PROTOCOL_TLS_SERVER", ssl.PROTOCOL_TLSv1_2)
    if mode == "tls10":
        protocol = ssl.PROTOCOL_TLSv1
    elif mode == "tls11":
        protocol = ssl.PROTOCOL_TLSv1_1
    context = ssl.SSLContext(protocol)
    alpn = hasattr(context, "set_alpn_protocols")
    if alpn:
        context.set_alpn_protocols(["h2", "http/1.1"])
    if mode in ("tls10", "tls11"):
        context.set_ciphers("DEFAULT:@SECLEVEL=0" if ssl.OPENSSL_VERSION_INFO >= (1, 1, 0) else "DEFAULT")
    context.load_cert_chain(
        os.path.join(directory, "server.pem"),
        os.path.join(directory, "server-key.pem"),
    )
    seen_names = []
    context.set_servername_callback(lambda connection, name, context: seen_names.append(name))
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)
        sys.stdout.write(str(listener.getsockname()[1]) + "\n")
        sys.stdout.flush()
        if mode.startswith("aria-"):
            aria_peer(listener, context, mode, seen_names)
            return
        connection, _ = listener.accept()
        with connection:
            connection.settimeout(10)
            if mode == "silent-write":
                connection.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
            if mode == "silent-handshake":
                time.sleep(5)
                return
            if mode == "reset":
                connection.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
                return
            try:
                secured = context.wrap_socket(connection, server_side=True)
            except ssl.SSLError:
                if mode in ("wrong-name", "wrong-ip", "untrusted", "tls10", "tls11",
                            "expired", "numeric-dns", "partial-wildcard"):
                    return
                raise
            with secured:
                if alpn and secured.selected_alpn_protocol() != "http/1.1":
                    raise RuntimeError("Client did not negotiate HTTP/1.1")
                expected_name = None if mode in ("ip", "wrong-ip") else "localhost"
                if ((expected_name is None and seen_names not in ([], [None])) or
                        (expected_name is not None and seen_names != [expected_name])):
                    raise RuntimeError("Unexpected TLS SNI")
                if mode in ("silent-read", "silent-write"):
                    time.sleep(5)
                    return
                if mode == "abrupt":
                    secured.sendall(b"x")
                    os.close(secured.detach())
                    return
                if mode == "large-write":
                    time.sleep(0.2)
                    data = receive(secured, 512 * 1024)
                    if data != bytes(bytearray(i % 251 for i in range(len(data)))):
                        raise RuntimeError("Corrupted or replayed client write")
                    secured.sendall(b"ok")
                else:
                    for _ in range(2):
                        if receive(secured, 4) != b"ping":
                            raise RuntimeError("Unexpected request")
                        for byte in b"pong":
                            secured.sendall(bytes(bytearray([byte])))
                # unwrap sends close_notify before waiting for the client's.
                # The adapter frees its connection without a shutdown round trip.
                try:
                    secured.unwrap()
                except (ssl.SSLError, ConnectionResetError):
                    pass


if __name__ == "__main__":
    main()
