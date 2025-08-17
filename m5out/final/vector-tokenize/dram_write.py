import pandas as pd
import os

results = []

for i in range(1, 99):
    filename = f"vector-tokenize-_{i}.txt"
    if not os.path.exists(filename):
        print(f"⚠️ 파일 없음: {filename}")
        continue

    found = False
    with open(filename, 'r') as f:
        for idx, line in enumerate(f):
            if idx >= 950:
                break
            if "system.mem_ctrls.dram.bytesWritten::total" in line:
                try:
                    value = int(line.strip().split()[1])
                    results.append((filename, value))
                    found = True
                    break
                except (IndexError, ValueError):
                    print(f"⚠️ {filename}에서 숫자 파싱 실패")
                    found = True
                    break

    if not found:
        print(f"⚠️ {filename}의 900줄 이내에 원하는 metric 없음")

# 결과 저장
df = pd.DataFrame(results, columns=["Filename", "DRAMBytesWritten"])
df.to_excel("dramwritten.xlsx", index=False)
print("✅ dramwritten.xlsx 저장 완료!")
