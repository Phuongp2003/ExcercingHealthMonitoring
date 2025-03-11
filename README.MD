# Hệ thống theo dõi sức khỏe khi vận động

## Mô tả
......

## Sử dụng (cho server flaskflask)

### Tạo venv (optional)
```bash
python -m venv myenv
```

### Cài các thư viện cần thiết
```bash
pip install -r .\pyserver\requirements.txt
```

### Khởi chạy server
```bash
python pyserver/app.py
```

### Hiển thị dư liệu thu được
```bash
py .\read_and_plot_sensor_data.py
```

### Production
```bash
gunicorn --bind 0.0.0.0:5000 wsgi:app
```

### Khởi chạy TCP server
```bash
python pyserver/app.py
```

### Debug version
```bash
python pyserver/debug.py
```

### Bắt đầu đo
```bash
curl http://localhost:5000/start
```

### Dừng đo
```bash
curl http://localhost:5000/stop
```

### Xem và tải các file đã ghi
Truy cập `http://localhost:5000/files` để xem và tải các file đã ghi.

### Thiết lập IP server cho ESP8266
Trong file `config.h`, thay thế `your_domain_name.com` bằng địa chỉ IP hoặc tên miền của server.
```cpp
const char* serverIP = "your_domain_name.com"; // Replace with your domain name
```
