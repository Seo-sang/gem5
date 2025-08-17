import pandas as pd

# 결과를 저장할 리스트 (각 항목은 딕셔너리 형태)
data = []

# 1 ~ 98까지 파일 이름 생성
for i in range(1, 99):
    filename = f"rv64gcv-ksc-{i}.txt"
    try:
        with open(filename) as f:
            lines = f.readlines()
            # 파일에 12번째 줄(인덱스 11)이 있는지 확인
            if len(lines) < 12:
                print(f"{filename}: 12번째 줄이 없습니다.")
                continue
            # 12번째 줄 추출 및 앞뒤 공백 제거
            line12 = lines[11].strip()
            # 공백 기준으로 분리 (필요에 따라 delimiter 수정 가능)
            parts = line12.split()
            if len(parts) < 2:
                print(f"{filename}: 12번째 줄에 숫자가 두 개 이상 없습니다.")
                continue

            # 두 숫자를 float 타입으로 변환 (정수이면 int() 또는 int(float()) 전환 가능)
            num1, num2 = float(parts[0]), float(parts[1])
            data.append(
                {"Filename": filename, "Number1": num1, "Number2": num2}
            )
    except Exception as e:
        print(f"파일 {filename} 처리 중 오류 발생: {e}")

# 데이터프레임 생성 후 엑셀 파일로 저장
df = pd.DataFrame(data)
df.to_excel("results.xlsx", index=False)
print("결과가 results.xlsx에 저장되었습니다.")
