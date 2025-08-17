MODEL="llama-2-7b-chat.Q4_0.gguf"
INPUT_DIR="wikitext-103"

for i in $(seq 1 98); do
    INPUT_FILE="${INPUT_DIR}/input_${i}.txt"
    echo "Processing $INPUT_FILE..."
    ./my-tokenize-sim -m "$MODEL" -f "$INPUT_FILE"
done
