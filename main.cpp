#include <memory>
#include <iostream>
#include <fstream>
#include <string>

#include <cstdint>
#include <cstring>
extern "C" {
#include <alsa/asoundlib.h>
}

struct DSDFormat {
    uint32_t sampling_rate;
    uint32_t num_channels;
    uint64_t total_samples;
    bool is_lsb_first;
};

class DSDReader {
public:
    virtual ~DSDReader() = default;
    virtual bool open(DSDFormat *format) = 0;
    virtual uint32_t read(uint8_t **data, uint32_t bytes_per_channel) = 0;
};

class DSFReader : public DSDReader {
    std::ifstream _in;
    uint8_t*_buf;
    uint32_t _ch, _blocksize, _filled, _pos;
    uint64_t _total_samples, _read_samples;
public:
    DSFReader(const std::string& path);
    ~DSFReader();
    bool open(DSDFormat *format);
    uint32_t read(uint8_t **data, uint32_t bytes);
};

uint8_t BIT_REVERSE_TABLE[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
void dsd64_to_dop(unsigned ch, bool is_lst_first, uint8_t **data, uint8_t *pcm, uint32_t bytes);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " [filename]" << std::endl;
        return 1;
    }
    const auto filename = std::string(argv[1]);
    auto reader = std::unique_ptr<DSDReader>(new DSFReader(filename));
    DSDFormat format;
    if (!reader->open(&format)) {
        std::cerr << "unknown file format" << std::endl;
        return 1;
    }
    std::cout << "DSD Format:" << std::endl
              << "  sampling rate: " << format.sampling_rate << std::endl
              << "  num channels:  " << format.num_channels << std::endl
              << "  lsb first?:    " << (format.is_lsb_first ? "yes" : "no") << std::endl
              << "  total samples: " << format.total_samples << std::endl;
    if (format.num_channels != 2) {
        std::cout << "not stereo" << std::endl;
        return 1;
    }
    auto alsa_buffer_size = 8192 * 4;
    auto blocksize = alsa_buffer_size / format.num_channels;
    uint8_t **buf = new uint8_t*[format.num_channels];
    for (auto i = 0u; i < format.num_channels; ++i)
        buf[i] = new uint8_t[blocksize];

    int err;
    snd_pcm_t *playback_handle = NULL;
    snd_pcm_hw_params_t *hw_params;
    uint8_t *pcm = new uint8_t[alsa_buffer_size];
    uint8_t **work = new uint8_t*[2];
    work[0] = new uint8_t[alsa_buffer_size >> 1];
    work[1] = new uint8_t[alsa_buffer_size >> 1];
    ::memset(pcm, 0, alsa_buffer_size);

    if ((err = snd_pcm_open (&playback_handle, "front:CARD=PMA50,DEV=0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "cannot open audio device" << std::endl;
        goto finalize;
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        std::cerr << "cannot allocate hardware parameter structure" << std::endl;
        goto finalize;
    }

    if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
        std::cerr << "cannot initialize hardware parameter structure" << std::endl;
        goto finalize;
    }

    if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::cerr << "cannot set access type" << std::endl;
        goto finalize;
    }

    if ((err = snd_pcm_hw_params_set_rate (playback_handle, hw_params, format.sampling_rate / 8 / 4, 0)) < 0) {
        std::cerr << "cannot set sample rate" << std::endl;
        goto finalize;
    }

    if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, format.num_channels)) < 0) {
        std::cerr << "cannot set channel count" << std::endl;
        goto finalize;
    }
    
    if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_DSD_U32_BE)) < 0) {
        std::cerr << "cannot set sample format" << std::endl;
        goto finalize;
    }

    snd_pcm_uframes_t frames;
    int dir;
    frames = alsa_buffer_size / format.num_channels / 4 /*32bit*/;
    snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &frames, &dir);
    alsa_buffer_size = frames;

    if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
        std::cerr << "cannot set parameters" << std::endl;
        goto finalize;
    }
    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (playback_handle)) < 0) {
        std::cerr << "cannot prepare audio interface for use" << std::endl;
        goto finalize;
    }

    while (1) {
        auto bytes = reader->read(work, alsa_buffer_size / format.num_channels);
        if (bytes == 0)
            break;
        auto i = 0;
        auto l = work[0], r = work[1];
        if (format.is_lsb_first) {
            for (auto j = 0u; j < bytes; j += 4) {
                pcm[i + 0] = BIT_REVERSE_TABLE[l[j + 0]];
                pcm[i + 1] = BIT_REVERSE_TABLE[l[j + 1]];
                pcm[i + 2] = BIT_REVERSE_TABLE[l[j + 2]];
                pcm[i + 3] = BIT_REVERSE_TABLE[l[j + 3]];
                pcm[i + 4] = BIT_REVERSE_TABLE[r[j + 0]];
                pcm[i + 5] = BIT_REVERSE_TABLE[r[j + 1]];
                pcm[i + 6] = BIT_REVERSE_TABLE[r[j + 2]];
                pcm[i + 7] = BIT_REVERSE_TABLE[r[j + 3]];
                i += 8;
            }
        } else {
            for (auto j = 0u; j < bytes; j += 4) {
                pcm[i + 0] = l[j + 0];
                pcm[i + 1] = l[j + 1];
                pcm[i + 2] = l[j + 2];
                pcm[i + 3] = l[j + 3];
                pcm[i + 4] = r[j + 0];
                pcm[i + 5] = r[j + 1];
                pcm[i + 6] = r[j + 2];
                pcm[i + 7] = r[j + 3];
                i += 8;
            }
        }

        int write_frames = bytes / 4/*32bit*/;
        if ((err = snd_pcm_writei (playback_handle, pcm, write_frames)) != write_frames) {
            std::cerr << "write to audio interface failed" << std::endl;
            break;
        }
    }

    std::cout << "OK" << std::endl;

 finalize:
    if (playback_handle)
        snd_pcm_close (playback_handle);
    if (pcm)
        delete[] pcm;
    return 0;
}

DSFReader::DSFReader(const std::string& path)
{
    _in.open(path, std::ios_base::in | std::ios_base::binary);
    if (!_in)
        throw std::runtime_error("file open failed");
    _buf = NULL;
    _blocksize = _filled = _pos = 0;
    _read_samples = 0;
}

DSFReader::~DSFReader()
{
    if (_buf) {
        delete[] _buf;
        _buf = NULL;
    }
}

bool DSFReader::open(DSDFormat *format)
{
    char ident[4];
    uint32_t i32;
    uint64_t size, i64;

    if (!_in.read (ident, 4) || ::memcmp(ident, "DSD ", 4) != 0 || !_in.read((char*)&size, 8))
        return false;
    if (!_in.seekg(size - 12, _in.cur))
        return false;
    if (!_in.read (ident, 4) || ::memcmp(ident, "fmt ", 4) != 0 || !_in.read((char*)&size, 8))
        return false;
    if (!_in.read ((char*)&i32, 4) || i32 != 1) // format version
        return false;
    if (!_in.read ((char*)&i32, 4) || i32 != 0) // format id
        return false;
    if (!_in.read ((char*)&i32, 4) || i32 != 2) // channel type
        return false;
    if (!_in.read ((char*)&i32, 4) || i32 != 2) // channel num
        return false;
    format->num_channels = i32;
    _ch = i32;
    if (!_in.read ((char*)&i32, 4) || (i32 != 2822400 && i32 != 5644800)) // sampling frequency
        return false;
    format->sampling_rate = i32;
    if (!_in.read ((char*)&i32, 4) || (i32 != 1 && i32 != 8)) // bits per sample
        return false;
    format->is_lsb_first = (i32 == 1);
    if (!_in.read ((char*)&i64, 8)) // sample count
        return false;
    format->total_samples = i64;
    _total_samples = i64;
    if (!_in.read ((char*)&i32, 4)) // block size per channel
        return false;
    _blocksize = i32;
    if (!_in.seekg(size - 48, _in.cur))
        return false;
    if (!_in.read (ident, 4) || ::memcmp(ident, "data", 4) != 0 || !_in.read((char*)&size, 8))
        return false;
    _buf = new uint8_t[_blocksize * format->num_channels];
    return true;
}

uint32_t DSFReader::read(uint8_t **data, uint32_t bytes)
{
    uint32_t read_bytes = 0;
    while (bytes > 0) {
        if (_pos == _filled) {
            if (!_in.read ((char*)_buf, _blocksize * _ch))
                return 0;
            _filled = (uint32_t)(_in.gcount() / _ch);
            _pos = 0;
        }
        auto size = std::min(bytes, _filled - _pos);
        for (auto i = 0u; i < _ch; ++i)
            ::memcpy(data[i], _buf + (_blocksize * i + _pos), size);
        _pos += size;
        bytes -= size;
        read_bytes += size;
    }
    _read_samples += read_bytes * 8;
    if (_read_samples > _total_samples) {
        read_bytes -= (_read_samples - _total_samples) / 8;
        _read_samples = _total_samples;
    }
    return read_bytes;
}

void dsd64_to_dop(unsigned ch, bool is_lst_first, uint8_t **data, uint8_t *pcm, uint32_t bytes)
{
    uint32_t pcm_idx = 0;
    for (uint32_t p = 0; p < (bytes & 0xfffffffe); p += 2) {
        for (unsigned c = 0; c < ch; ++c) {
            uint8_t *x = (uint8_t*)(pcm + pcm_idx * 3);
            x[0] = 0x05;
            if (is_lst_first) {
                x[1] = BIT_REVERSE_TABLE[data[c][p + 0]];
                x[2] = BIT_REVERSE_TABLE[data[c][p + 1]];
            } else {
                x[1] = data[c][p + 0];
                x[2] = data[c][p + 1];
            }
            ++pcm_idx;
        }
    }

    if ((bytes & 1) != 0)
        throw std::invalid_argument("bytes must be power of 2");
}
