    .section .text
    .globl vstrcmp
    .type vstrcmp, @function
vstrcmp:
    addi sp, sp, -48        # 스택 공간 할당
    sd ra, 40(sp)
    sd s0, 32(sp)            # ra 저장
    #sd a1, 24(sp)
    li t2, 1
    vsetivli t0, 8, e8, m1, tu, mu
vstrcmp_loop:
    vle8.v v1, (a0)         # str1 로드
    vle8.v v2, (a1)         # str2 로드
    vstrcmp.vv v3, v1, v2     # 벡터 비교
    vmv.x.s t1, v3          # 결과를 t1에 옮김
    beq t1, t2, noteq       # 만약 t1 == t2 (즉, 비교 결과가 "같음" 표시)면 noteq로 분기
    ld ra, 40(sp)
    ld s0, 32(sp)            # ra 저장
    #ld a1, 24(sp)
    addi sp, sp, 48
    mv a0, t1
    ret
noteq:
    add a0, a0, t0          # 포인터 업데이트: str1 이동
    add a1, a1, t0          # 포인터 업데이트: str2 이동
    j vstrcmp_loop          # 다시 비교
