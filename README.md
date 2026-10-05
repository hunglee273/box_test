# mcu_test – Hướng dẫn test xi lanh và động cơ bước

Firmware test phần cứng cho board **NUCLEO-H753ZI**. Mọi thao tác test được thực hiện bằng lệnh gõ qua Serial Monitor.

Tài liệu này hướng dẫn test hai nhóm thiết bị:

- **Xi lanh điện** (2 cái): driver Cytron SmartDriveDuo-30 (MDDS30).
- **Động cơ bước** (3 cái): MISUMI E-42ESTM01 + driver EDR42A (closed loop).

---

## 1. Chuẩn bị

### 1.1. Mở Serial Monitor (VS Code)

| Thông số | Giá trị |
|---|---|
| Cổng | `/dev/ttyACM0` (ST-Link Virtual COM) |
| Baud | `115200` |
| Line ending | `CRLF` hoặc `LF` |

Nhấn nút RESET trên Nucleo. Nếu kết nối được, màn hình hiện `=== MCU TEST FIRMWARE ===` và dấu nhắc `>`.

Mỗi lệnh gõ trên một dòng rồi nhấn **Enter**. Tên thiết bị không phân biệt hoa thường.

### 1.2. Lệnh chung

| Lệnh | Tác dụng |
|---|---|
| `help` | danh sách lệnh chung |
| `help cyl` / `help step` | hướng dẫn chi tiết của từng module, in ngay trên board |
| `list` | liệt kê các module đang có |
| `stop` | **dừng khẩn cấp**: dừng tất cả xi lanh và động cơ bước |

### 1.3. Thứ tự cấp nguồn

1. Cắm USB cho Nucleo trước, để STM32 khởi động xong.
2. Sau đó mới bật nguồn MDDS30 và driver động cơ bước.

Làm theo thứ tự này thì lúc driver bật lên, chân PWM đã ở mức 0. Nếu không, MDDS30 sẽ báo lỗi Input Error.

---

## 2. Xi lanh – MDDS30

### 2.1. Đấu dây

| Xi lanh | Tên lệnh | PWM (tốc độ) | DIR (chiều) | Ngõ ra MDDS30 |
|---|---|---|---|---|
| Trái | `L` | PD14 (TIM4_CH3) → **AN1** | PF2 → **IN1** | MOTOR LEFT |
| Phải | `R` | PA5 (TIM2_CH1) → **AN2** | PF3 → **IN2** | MOTOR RIGHT |

- GND của STM32 phải nối chung với GND tín hiệu của MDDS30.
- Tần số PWM: 333 Hz.

### 2.2. Kiểm tra driver không cần MCU

- Nhấn nút **MLA / MLB**: xi lanh trái phải chạy.
- Nhấn nút **MRA / MRB**: xi lanh phải phải chạy.

Nếu xi lanh không chạy ở bước này, lỗi nằm ở nguồn, dây động cơ hoặc driver, không phải ở firmware.

### 2.3. Lệnh

| Lệnh | Tác dụng |
|---|---|
| `cyl` | trạng thái 2 xi lanh: đang làm gì, chiều, duty, mức chân DIR, thời gian còn lại |
| `cyl <L\|R\|all> ext [duty] [ms]` | đẩy ra |
| `cyl <L\|R\|all> ret [duty] [ms]` | rút về |
| `cyl <L\|R\|all> stop` | dừng ngay |

- `duty`: 0–100 %, mặc định **50 %**.
- `ms`: 1–30000, mặc định **2000 ms**.

Cơ chế bảo vệ có sẵn trong firmware:

- **Mỗi lệnh luôn có thời gian giới hạn.** Hết thời gian thì xi lanh tự dừng, kể cả khi bạn quên gõ `stop`.
- **Đổi chiều khi đang chạy:** xi lanh tự dừng hẳn 100 ms rồi mới đảo chiều.
- **Khởi động mềm:** duty tăng dần từ 0 lên giá trị đặt trong 200 ms.

### 2.5. Trình tự test

| Bước | Lệnh | Kết quả mong đợi |
|---|---|---|
| 1 | `cyl` | cả L và R ở trạng thái `idle`, duty 0 %. Đèn ERR trên MDDS30 tắt |
| 2 | `cyl L ext 50 10000` | xi lanh trái chạy chậm 1 s rồi tự dừng, in `[cyl] L dung (het 1000 ms)` |
| 3 | `cyl L ret 50 10000` | xi lanh trái chạy chiều ngược lại 1 s |
| 4 | `cyl R ext 50 10000` rồi `cyl R ret 50 10000` | tương tự với xi lanh phải |
| 5 | `cyl L ext 50 10000`, sau 1–2 s gõ `cyl L stop` | xi lanh dừng ngay khi gõ lệnh |
| 6 | `cyl L ext 50 10000`, đang chạy gõ `cyl L ret 50 10000` | dừng khoảng 100 ms rồi đảo chiều |
| 7 | `cyl all ext 50 10000` | cả hai cùng đẩy ra |
| 8 | `cyl L ext 60 10000`, rồi `80`, rồi `100` | tốc độ tăng dần |

> **Lưu ý về tốc độ tối đa:** MDDS30 lọc PWM thành điện áp, và full tốc độ tương ứng 5 V. Vì chân STM32 chỉ ra 3.3 V, duty 100 % chỉ đạt khoảng **66 %** tốc độ thật.

### 2.6. Xử lý sự cố

| Hiện tượng | Nguyên nhân / cách xử lý |
|---|---|
| Đèn **ERR nháy 2 lần** | Input Error: lúc MDDS30 bật nguồn, chân AN không ở 0 V. Nhấn RESET trên MDDS30 sau khi STM32 đã chạy. Nên gắn điện trở 10 kΩ kéo AN1/AN2 xuống GND |
| ERR nháy 3 / 4 / 5 lần | lần lượt là thấp áp / quá áp (> 35 V) / quá nhiệt |
| Đèn **OC** sáng | quá dòng: xi lanh bị kẹt hoặc đã chạm cuối hành trình |
| Lệnh in đúng nhưng xi lanh không chạy | kiểm tra DIP (SW1 = ON, SW2 = OFF, SW6 = ON), dây AN/IN, GND chung |
| Chỉ chạy được một chiều | dây IN1/IN2 lỏng, hoặc SW6 đang OFF |
| `ext` lại rút về | đổi `CYL_DIR_EXTEND_LEVEL` thành `GPIO_PIN_RESET` trong `App/board_config.h` |

---

## 3. Động cơ bước – EDR42A (đấu PNP)

### 3.1. Đấu dây (PNP / cathode chung)

```
MCU ──► PUL+   DIR+   ENA+
GND ─────────────────────► PUL-   DIR-   ENA-
ALM+ ──► chân ALM của MCU (đã có pull-up)
ALM- ──► GND
```

| Động cơ | Tên lệnh | PUL+ | DIR+ | ENA+ | ALM+ |
|---|---|---|---|---|---|
| Ngang 1 | `H1` | PE14 (TIM1_CH4) | PF5 | PF4 | PG5 |
| Đứng | `V` | PE13 (TIM1_CH3) | PF7 | PF6 | PG6 |
| Ngang 2 | `H2` | PE9 (TIM1_CH1) | PF1 | PE2 | PG7 |

> **An toàn:** firmware **chưa** dừng theo công tắc hành trình (các cảm biến `LM_SW*`). Khi test lần đầu, nên tách động cơ khỏi cơ cấu, hoặc đưa trục về giữa hành trình, và dùng số bước nhỏ.

### 3.2. Lệnh

| Lệnh | Tác dụng |
|---|---|
| `step` | trạng thái 3 động cơ: pos, tốc độ, ENA, ALM (kèm mức chân H/L) |
| `step io` | mức chân thực tế và trạng thái opto, dùng để kiểm tra dây |
| `step <ten> move <buoc> [sps] [acc]` | chạy N bước. Số âm là ngược chiều |
| `step <ten> rev <vong> [vong/s]` | chạy theo số vòng (dùng `STEPPER_PPR`) |
| `step <ten> run <+-sps> [acc]` | chạy liên tục cho đến khi có lệnh `stop` |
| `step <ten\|all> stop` | giảm tốc rồi dừng |
| `step <ten\|all> halt` | dừng ngay, không giảm tốc |
| `step <ten\|all> ena on\|off` | bật driver (giữ trục) / tắt driver (thả trục) |
| `step <ten> zero` | đặt `pos = 0` |

- `<ten>`: `H1`, `V`, `H2`.
- `sps`: bước/giây, phạm vi 10–50000, mặc định 1600.
- `acc`: gia tốc (bước/giây²), mặc định 3200.

Đặc điểm khi chạy:

- **Ba động cơ chạy độc lập cùng lúc**, mỗi cái có tốc độ, gia tốc và số bước riêng.
- Tăng tốc và giảm tốc theo hình thang.
- **Bật ENA tự động:** khi gặp lệnh `move` / `rev` / `run` mà driver đang tắt, firmware tự bật ENA, chờ 200 ms rồi mới phát xung.
- **Lệnh `stop` chung chỉ dừng xung, không tắt ENA.** Nhờ vậy tải trên trục `V` (trục đứng) không bị rơi.

### 3.3. Trình tự test (làm lần lượt cho H1, V, H2)

**Bước 1 – Kiểm tra tín hiệu**

```
step io
```

Mong đợi:

- `PUL=L`
- `ENA=H(opto on -> driver TAT)`: lúc khởi động driver đang tắt.
- `ALM=...(ok)`

**Bước 2 – Kiểm tra ENA**

| Lệnh | Kết quả mong đợi |
|---|---|
| `step H1 ena on` | trục **cứng**, không xoay tay được |
| `step H1 ena off` | trục **lỏng**, xoay tay được |

**Bước 3 – Chạy thử và đo số xung/vòng (PPR)**

| Bước | Lệnh | Kết quả mong đợi |
|---|---|---|
| 1 | `step H1 move 200 10000` | quay chậm khoảng 1 s, in `[step] H1 xong 200 buoc ... pos=200` |
| 2 | đánh dấu trục, rồi gõ `step H1 move 200 10000` |  |
| 3 | sửa `STEPPER_PPR` trong `App/board_config.h`, build và nạp lại | |
| 4 | `step H1 rev 1 10` | quay đúng **1 vòng** trong khoảng 10 s |
| 5 | `step H1 rev -1 10` | quay ngược 1 vòng, `pos` về 0 |

Ví dụ ở bước 2: nếu trục quay được ½ vòng thì PPR = 3200.

**Bước 4 – Dừng và tốc độ**

| Bước | Lệnh | Kết quả mong đợi |
|---|---|---|
| 1 | `step H1 run 1600`, vài giây sau gõ `step H1 stop` | quay liên tục, rồi giảm tốc êm khi dừng |
| 2 | `step H1 run 1600` rồi `step H1 halt` | dừng ngay |
| 3 | `step H1 rev 5 2`, rồi `rev 5 4`, rồi `rev 5 8` | quay nhanh dần. Mức tốc độ bắt đầu kêu, rung hoặc mất bước là giới hạn |

**Bước 5 – Chạy ba động cơ cùng lúc**

```
step H1 run 1600
step V run -800
step H2 run 3200
step
stop
```

Lệnh `step` ở giữa dùng để xem cả ba động cơ đang chạy với tốc độ khác nhau. Lệnh `stop` cuối cùng dừng tất cả.

> **Trục V (trục đứng):** không gõ `step V ena off` khi đang có tải treo.

### 3.4. Xử lý sự cố

| Hiện tượng | Nguyên nhân / cách xử lý |
|---|---|
| Lệnh in `xong` nhưng trục không quay | tín hiệu không đủ 5 V (thiếu hoặc lỗi mạch đệm), sai dây PUL hoặc GND, hoặc driver đang tắt (xem `step io`) |
| Trục quay rất chậm, gần như đứng yên | PPR thực tế lớn. Đo lại theo bước 3 |
| `ena on` thì trục lỏng, `ena off` thì trục cứng | đặt `STEPPER_ENA_OPTO_DISABLES 0` |
| `step io` báo `ALM=LOI` khi driver vẫn bình thường | đổi `STEPPER_ALM_ACTIVE_LEVEL` thành `GPIO_PIN_SET` |
| Quay sai chiều | đổi cột cuối (`0` → `1`) của động cơ đó trong `STEPPER_LIST` |
| Đèn driver đỏ / in `[step] H1 ALM BAO LOI` | driver báo lỗi (quá dòng, lệch vị trí...). Tắt nguồn, kiểm tra cơ khí, giảm `sps` hoặc `acc` |
| Rung hoặc mất bước khi chạy nhanh | giảm tốc độ hoặc gia tốc, ví dụ `step H1 move 3200 1600 1000` |
| Báo `ERR: H1 dang chay` | đang có lệnh khác chạy. Gõ `step H1 stop` trước |

---

## 4. Cấu hình (`App/board_config.h`)

Sau khi sửa file này, cần build và nạp lại firmware.

**Xi lanh**

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `CYL_DIR_EXTEND_LEVEL` | `GPIO_PIN_SET` | mức chân IN khi đẩy ra |
| `CYL_DUTY_DEFAULT` / `CYL_DUTY_MAX` | 50 / 100 | duty mặc định / tối đa (%) |
| `CYL_T_DEFAULT_MS` / `CYL_T_MAX_MS` | 2000 / 30000 | thời gian chạy mặc định / tối đa (ms) |
| `CYL_RAMP_MS` | 200 | thời gian khởi động mềm (ms) |
| `CYL_DEADTIME_MS` | 100 | thời gian dừng trước khi đổi chiều (ms) |

**Động cơ bước**

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `STEPPER_COMMON_ANODE` | 0 | 0 = PNP, push-pull, opto dẫn khi chân HIGH. 1 = NPN, open-drain |
| `STEPPER_ENA_OPTO_DISABLES` | 1 | 1 = opto ENA dẫn thì driver **tắt** |
| `STEPPER_ALM_ACTIVE_LEVEL` | `GPIO_PIN_RESET` | mức chân ALM khi driver báo lỗi |
| `STEPPER_ALM_STOP` | 0 | 1 = tự dừng và chặn lệnh khi ALM báo lỗi. Chỉ bật sau khi đã xác nhận đúng mức ALM |
| `STEPPER_PPR` | 1600 | số xung/vòng theo DIP. **Chưa xác nhận**, đo theo mục 3.3 |
| `STEPPER_SPS_DEFAULT` / `STEPPER_SPS_MAX` | 1600 / 50000 | tốc độ mặc định / tối đa (bước/s) |
| `STEPPER_ACC_DEFAULT` | 3200 | gia tốc mặc định (bước/s²) |
| `STEPPER_LIST` (cột cuối) | 0 | 1 = đảo chiều quay của động cơ đó |
