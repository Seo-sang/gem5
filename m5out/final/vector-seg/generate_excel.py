import pandas as pd
import os

results = []

# 파일 반복: 1 ~ 98
for i in range(1, 99):
    filename = f"vector-seg-_{i}.txt"
    if not os.path.exists(filename):
        print(f"⚠️ 파일 없음: {filename}")
        continue
    
    with open(filename, 'r') as f:
        lines = f.readlines()

    # 15번째 줄은 index 14 (0-based)
    if len(lines) >= 15:
        line = lines[14]
        if "system.cpu.numCycles" in line:
            num = int(line.strip().split()[1])
            results.append((filename, num))
        else:
            print(f"⚠️ {filename}의 15번째 줄에 numCycles 없음")
    else:
        print(f"⚠️ {filename} 줄 수 부족")

# 엑셀로 저장
df = pd.DataFrame(results, columns=["Filename", "numCycles"])
df.to_excel("numCycles_summary.xlsx", index=False)

print("✅ numCycles_summary.xlsx 저장 완료!")
