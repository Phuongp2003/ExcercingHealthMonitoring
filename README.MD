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
