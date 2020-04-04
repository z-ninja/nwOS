#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_byte_swap.h>
#include <nw_system/endian.h>

NW_PUBLIC uint16_t nw_swap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8 );
}
NW_PUBLIC uint16_t nw_readUint16LE( uint16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint16(val);
    #endif
}
NW_PUBLIC uint16_t nw_writeUint16LE( uint16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint16(val);
    #endif
}


NW_PUBLIC uint16_t nw_readUint16BE( uint16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint16(val);
    #else
    return val;
    #endif
}
NW_PUBLIC uint16_t nw_writeUint16BE( uint16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint16(val);
    #else
    return val;
    #endif
}

NW_PUBLIC int16_t nw_swap_int16( int16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}


NW_PUBLIC int16_t nw_readInt16LE( int16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int16(val);
    #endif
}
NW_PUBLIC int16_t nw_writeInt16LE( int16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int16(val);
    #endif
}


NW_PUBLIC int16_t nw_readInt16BE( int16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int16(val);
    #else
    return val;
    #endif
}


NW_PUBLIC int16_t nw_writeInt16BE( int16_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int16(val);
    #else
    return val;
    #endif
}


NW_PUBLIC uint32_t nw_swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

NW_PUBLIC uint32_t nw_readUint32LE( uint32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint32(val);
    #endif
}
NW_PUBLIC uint32_t nw_writeUint32LE( uint32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint32(val);
    #endif
}

NW_PUBLIC uint32_t nw_readUint32BE( uint32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint32(val);
    #else
    return val;
    #endif
}
NW_PUBLIC uint32_t nw_writeUint32BE( uint32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint32(val);
    #else
    return val;
    #endif
}


NW_PUBLIC int32_t nw_swap_int32( int32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

NW_PUBLIC int32_t nw_readInt32LE( int32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int32(val);
    #endif
}
NW_PUBLIC int32_t nw_writeInt32LE( int32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int32(val);
    #endif
}


NW_PUBLIC int32_t nw_readInt32BE( int32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int32(val);
    #else
    return val;
    #endif
}


NW_PUBLIC int32_t nw_writeInt32BE( int32_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int32(val);
    #else
    return val;
    #endif
}

NW_PUBLIC uint64_t nw_swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

NW_PUBLIC uint64_t nw_readUint64LE( uint64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint64(val);
    #endif
}
NW_PUBLIC uint64_t nw_writeUint64LE( uint64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_uint64(val);
    #endif
}

NW_PUBLIC uint64_t nw_readUint64BE( uint64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint64(val);
    #else
    return val;
    #endif
}
NW_PUBLIC uint64_t nw_writeUint64BE( uint64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_uint64(val);
    #else
    return val;
    #endif
}



NW_PUBLIC int64_t nw_swap_int64( int64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

NW_PUBLIC int64_t nw_readInt64LE( int64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int64(val);
    #endif
}
NW_PUBLIC int64_t nw_writeInt64LE( int64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_int64(val);
    #endif
}


NW_PUBLIC int64_t nw_readInt64BE( int64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int64(val);
    #else
    return val;
    #endif
}


NW_PUBLIC int64_t nw_writeInt64BE( int64_t val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_int64(val);
    #else
    return val;
    #endif
}


NW_PUBLIC float nw_swap_float(float x)
{
    union
    {
        float f;
        uint32_t ui32;
    } swapper;
    swapper.f = x;
    swapper.ui32 = nw_swap_uint32(swapper.ui32);
    return swapper.f;
}


NW_PUBLIC float nw_readFloatLE( float val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_float(val);
    #endif
}
NW_PUBLIC float nw_writeFloatLE( float val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_float(val);
    #endif
}


NW_PUBLIC float nw_readFloatBE( float val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_float(val);
    #else
    return val;
    #endif
}


NW_PUBLIC float nw_writeFloatBE( float val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_float(val);
    #else
    return val;
    #endif
}




NW_PUBLIC double nw_swap_double(double x)
{
    union
    {
        double f;
        uint64_t ui64;
    } swapper;
    swapper.f = x;
    swapper.ui64 = nw_swap_uint64(swapper.ui64);
    return swapper.f;
}


NW_PUBLIC double nw_readDoubleLE( double val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_double(val);
    #endif
}
NW_PUBLIC double nw_writeDoubleLE( double val )
{
    #ifdef NW_LITTLE_ENDIAN
    return val;
    #else
    return nw_swap_double(val);
    #endif
}


NW_PUBLIC double nw_readDoubleBE( double val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_double(val);
    #else
    return val;
    #endif
}


NW_PUBLIC double nw_writeDoubleBE( double val )
{
    #ifdef NW_LITTLE_ENDIAN
    return nw_swap_double(val);
    #else
    return val;
    #endif
}





