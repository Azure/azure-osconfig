# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""Loopback-only TLS peer for owner-run tests; generates temporary test keys."""

import os
import signal
import socket
import ssl
import struct
import subprocess
import sys
import time


def certificate(openssl, directory, name, subject):
    subprocess.check_call(
        [
            openssl, "req", "-new", "-x509", "-newkey", "rsa:2048", "-nodes",
            "-days", "1", "-sha256", "-subj", "/CN=" + subject,
            "-config", os.path.join(directory, "openssl.cnf"),
            "-keyout", os.path.join(directory, name + "-key.pem"),
            "-out", os.path.join(directory, name + ".pem"),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        timeout=10,
    )


def receive(connection, count):
    result = bytearray()
    while len(result) < count:
        part = connection.recv(min(16384, count - len(result)))
        if not part:
            raise RuntimeError("Client disconnected before completing its request")
        result.extend(part)
    return bytes(result)


def main():
    openssl, directory, mode = sys.argv[1:]
    os.umask(0o077)
    name = "wrong.example" if mode == "wrong-name" else "localhost"
    with open(os.path.join(directory, "openssl.cnf"), "w") as config:
        config.write(
            "[req]\ndistinguished_name=dn\nx509_extensions=extensions\n"
            "[dn]\n[extensions]\nbasicConstraints=critical,CA:TRUE\n"
            "keyUsage=critical,digitalSignature,keyEncipherment,keyCertSign\n"
            "extendedKeyUsage=serverAuth\nsubjectAltName=DNS:" + name + ",IP:127.0.0.1\n"
        )
    certificate(openssl, directory, "server", name)
    if mode == "untrusted":
        certificate(openssl, directory, "root", "Other Test Root")
    else:
        with open(os.path.join(directory, "server.pem"), "rb") as source:
            with open(os.path.join(directory, "root.pem"), "wb") as target:
                target.write(source.read())

    signal.signal(signal.SIGALRM, signal.SIG_DFL)
    signal.alarm(20)
    protocol = getattr(ssl, "PROTOCOL_TLS_SERVER", ssl.PROTOCOL_TLSv1_2)
    if mode == "tls10":
        protocol = ssl.PROTOCOL_TLSv1
    context = ssl.SSLContext(protocol)
    alpn = hasattr(context, "set_alpn_protocols")
    if alpn:
        context.set_alpn_protocols(["h2", "http/1.1"])
    if mode == "tls10":
        context.set_ciphers("DEFAULT:@SECLEVEL=0")
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
                if mode in ("wrong-name", "wrong-ip", "untrusted", "tls10"):
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
