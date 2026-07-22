#!/bin/bash
# ESP32S3 Sense 动态局域网IP HTTPS 证书生成脚本
# 输出 pem 格式，兼容 esp-idf mbedtls

OUT_DIR="./esp32_ssl_cert"
mkdir -p $OUT_DIR

# 1. 生成CA根私钥与根证书（有效期10年）
openssl genrsa -out $OUT_DIR/ca.key 2048
openssl req -x509 -new -nodes -key $OUT_DIR/ca.key -sha256 -days 3650 -out $OUT_DIR/ca.crt \
-subj "/C=CN/ST=Local/L=Lan/O=ESP32S3/OU=Device/CN=ESP32-CA-Root"

# 2. 生成设备私钥与证书请求CSR
openssl genrsa -out $OUT_DIR/device.key 2048
openssl req -new -key $OUT_DIR/device.key -out $OUT_DIR/device.csr \
-subj "/C=CN/ST=Local/L=Lan/O=XiaoS3Sense/OU=HTTPS/CN=esp32s3.local"

# 3. SAN扩展配置：支持任意局域网IP + mDNS域名
cat > $OUT_DIR/san.ext << EOF
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_req
prompt = no
[req_distinguished_name]
C = CN
ST = Local
L = Lan
O = XiaoS3Sense
OU = HTTPS
CN = esp32s3.local
[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names
[alt_names]
DNS.1 = esp32s3.local
DNS.2 = *.local
# IP占位段，覆盖全部内网网段 192.168.x.x / 10.x.x.x / 172.16-31.x.x
IP.1 = 192.168.0.0
IP.2 = 10.0.0.0
IP.3 = 172.16.0.0
IP.4 = 127.0.0.1
EOF

# 4. CA签发设备证书（有效期5年）
openssl x509 -req -in $OUT_DIR/device.csr -CA $OUT_DIR/ca.crt -CAkey $OUT_DIR/ca.key \
-CAcreateserial -out $OUT_DIR/device.crt -days 1825 -sha256 -extfile $OUT_DIR/san.ext

# 5. 合并为ESP32直接使用的PEM（证书+私钥一体）
cat $OUT_DIR/device.crt $OUT_DIR/device.key > $OUT_DIR/esp32_server.pem

# 6. 转换根CA为PEM（电脑浏览器/客户端信任用）
cp $OUT_DIR/ca.crt $OUT_DIR/ca_root.pem

echo "===== 生成完成 ====="
echo "设备烧录用：esp32_server.pem (证书+私钥)"
echo "电脑信任根证书：ca_root.pem"
ls -lh $OUT_DIR/

