
#ifndef MAX30003_DEFINE_H_
#define MAX30003_DEFINE_H_

typedef enum
{

    MAX30003_NO_OP = 0x00,
    MAX30003_STAUS = 0x01,
    MAX30003_EN_INT = 0x02,
    MAX30003_EN_INT2 = 0x03,
    MAX30003_MNGR_INT = 0x04,
    MAX30003_MNGR_DYN = 0x05,
    MAX30003_SW_RST = 0x08,
    MAX30003_SYNCH = 0x09,
    MAX30003_FIFO_RST = 0x0A,
    MAX30003_INFO = 0x0F,
    MAX30003_CNFG_GEN = 0x10,
    MAX30003_CNFG_CAL = 0x12,
    MAX30003_CNFG_EMUX = 0x14,
    MAX30003_CNFG_ECG = 0x15,
    MAX30003_CNFG_RTOR1 = 0x1D,
    MAX30003_CNFG_RTOR2 = 0x1E,
    MAX30003_ECG_BURST = 0x20,
    MAX30003_ECG = 0x21,
    MAX30003_RTOR = 0x25

} max30003_enum;

typedef union
{

    struct
    {

        uint32_t LDOFF_NL :1;
        uint32_t LDOFF_NH :1;
        uint32_t LDOFF_PL :1;
        uint32_t LDOFF_PH :1;
        uint32_t RESERVED0 :4;
        uint32_t PLLINT :1;
        uint32_t SAMP :1;
        uint32_t RRINT :1;
        uint32_t LONINT :1;
        uint32_t RESERVED1 :8;
        uint32_t DCLOFFINT :1;
        uint32_t FSTINT :1;
        uint32_t EOVF :1;
        uint32_t EINT :1;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_status_reg;

typedef union
{

    struct
    {

        uint32_t INTB_TYPE :2;
        uint32_t RESERVED0 :6;
        uint32_t EN_PLLINT :1;
        uint32_t EN_SAMP :1;
        uint32_t EN_RRINT :1;
        uint32_t EN_LONINT :1;
        uint32_t RESERVED1 :8;
        uint32_t EN_DCLOFFINT :1;
        uint32_t EN_FSTINT :1;
        uint32_t EN_EOVF :1;
        uint32_t EN_EINT :1;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_en_int_reg;

typedef union
{

    struct
    {

        uint32_t INTB_TYPE :2;
        uint32_t RESERVED0 :6;
        uint32_t EN_PLLINT :1;
        uint32_t EN_SAMP :1;
        uint32_t EN_RRINT :1;
        uint32_t EN_LONINT :1;
        uint32_t RESERVED1 :8;
        uint32_t EN_DCLOFFINT :1;
        uint32_t EN_FSTINT :1;
        uint32_t EN_EOVF :1;
        uint32_t EN_EINT :1;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_en_int2_reg;

typedef union
{

    struct
    {

        uint32_t SAMP_IT :2;
        uint32_t CLR_SAMP :1;
        uint32_t RESERVED0 :1;
        uint32_t CLR_RRINT :2;
        uint32_t CLR_FAST :1;
        uint32_t RESERVED1 :12;
        uint32_t EFIT :5;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_mngr_int_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :16;
        uint32_t FAST_TH :6;
        uint32_t FAST :2;
        uint32_t RESERVED1 :8;

    } bits;

    uint32_t value;

} max30003_mngr_dyn_reg;

typedef union
{

    struct
    {

        uint32_t SW_RST :24;
        uint32_t RESERVED0 :8;

    } bits;

    uint32_t value;

} max30003_sw_rst_reg;

typedef union
{

    struct
    {

        uint32_t SYNCH :24;
        uint32_t RESERVED0 :8;

    } bits;

    uint32_t value;

} max30003_synch_reg;

typedef union
{

    struct
    {

        uint32_t FIFO_RST :24;
        uint32_t RESERVED0 :8;

    } bits;

    uint32_t value;

} max30003_fifo_rst_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :16;
        uint32_t REV_ID :4;
        uint32_t RESERVED1 :4;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_info_reg;

typedef union
{

    struct
    {

        uint32_t RBIASN :1;
        uint32_t RBIASP :1;
        uint32_t RBIASV :2;
        uint32_t EN_RBIAS :2;
        uint32_t VTH :2;
        uint32_t IMAG :3;
        uint32_t IPOL :1;
        uint32_t EN_DCLOFF :2;
        uint32_t RESERVED0 :5;
        uint32_t EN_ECG :1;
        uint32_t FMSTR :2;
        uint32_t EN_ULP_LON :2;
        uint32_t RESERVED1 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_gen_reg;

typedef union
{

    struct
    {

        uint32_t THIGH :11;
        uint32_t FIFTY :1;
        uint32_t FCAL :3;
        uint32_t RESERVED0 :5;
        uint32_t VMAG :1;
        uint32_t VMODE :1;
        uint32_t EN_VCAL :1;
        uint32_t RESERVED1 :1;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_cal_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :16;
        uint32_t CALN_SEL :2;
        uint32_t CALP_SEL :2;
        uint32_t OPENN :1;
        uint32_t OPENP :1;
        uint32_t RESERVED1 :1;
        uint32_t POL :1;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_emux_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :12;
        uint32_t DLPF :2;
        uint32_t DHPF :1;
        uint32_t RESERVED1 :1;
        uint32_t GAIN :2;
        uint32_t RESERVED2 :4;
        uint32_t RATE :2;
        uint32_t RESERVED3 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_ecg_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :8;
        uint32_t PTSF :4;
        uint32_t PAVG :2;
        uint32_t RESERVED1 :1;
        uint32_t EN_RTOR :1;
        uint32_t GAIN :4;
        uint32_t WNDW :4;
        uint32_t RESERVED2 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_rtor_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :8;
        uint32_t RHSF :3;
        uint32_t RESERVED1 :1;
        uint32_t RAVG :2;
        uint32_t RESERVED3 :2;
        uint32_t HOFF :6;
        uint32_t RESERVED4 :2;
        uint32_t RESERVED5 :8;

    } bits;

    uint32_t value;

} max30003_cnfg_rtor2_reg;

typedef union
{

    struct
    {

        uint32_t PTAG :3;
        uint32_t ETAG :3;
        uint32_t ECG_SAMPLE :18;
        uint32_t RESERVED0 :8;

    } bits;

    uint32_t value;

} max30003_ecg_burst_reg;

typedef union
{

    struct
    {

        uint32_t PTAG :3;
        uint32_t ETAG :3;
        uint32_t ECG_SAMPLE :18;
        uint32_t RESERVED0 :8;

    } bits;

    uint32_t value;

} max30003_ecg_reg;

typedef union
{

    struct
    {

        uint32_t RESERVED0 :10;
        uint32_t RTOR_INTERVAL :14;
        uint32_t RESERVED1 :8;

    } bits;

    uint32_t value;

} max30003_rtor_reg;

#endif /* MAX30003_DEFINE_H_ */
