from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl
import socket

class CORSRequestHandler(SimpleHTTPRequestHandler):
    # Override extensions_map to fix MIME types for ES modules and WASM
    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        '.mjs': 'application/javascript',
        '.wasm': 'application/wasm',
        '.tflite': 'application/octet-stream',
        '.onnx': 'application/octet-stream',
    }

    def end_headers(self):
        # CORS headers for local development
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

    def handle(self):
        """Override handle to catch SSL connection errors gracefully."""
        try:
            super().handle()
        except (ConnectionAbortedError, ConnectionResetError, BrokenPipeError, ssl.SSLError) as e:
            # Client disconnected during transfer - silently ignore
            pass
        except Exception as e:
            print(f"[ERROR] {e}")

    def handle_one_request(self):
        """Override to catch SSL errors during request processing."""
        try:
            super().handle_one_request()
        except (ConnectionAbortedError, ConnectionResetError, BrokenPipeError, ssl.SSLError):
            pass

    def log_message(self, format, *args):
        # Suppress stack traces for normal operations
        print(f"{self.client_address[0]} - {format % args}")

port = 8080
server_address = ("localhost", port)

httpd = HTTPServer(server_address, CORSRequestHandler)
# Wrap with SSL
httpd.socket = ssl.wrap_socket(
    httpd.socket,
    server_side=True,
    certfile="server.crt",
    keyfile="server.key"
)
print(f"HTTPS 服务启动：https://localhost:{port}")
print(f"已注册 MIME: .mjs→application/javascript, .wasm→application/wasm")
try:
    httpd.serve_forever()
except KeyboardInterrupt:
    print("\n服务已停止")
    httpd.server_close()