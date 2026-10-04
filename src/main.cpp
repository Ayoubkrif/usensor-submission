#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

constexpr uint64_t kMs = 1'000'000ULL;
constexpr uint64_t kWindowCameraNs = 50 * kMs;
constexpr uint64_t kWindowImuNearestNs = 20 * kMs;
constexpr uint64_t kWindowImuAroundNs = 100 * kMs;
constexpr uint64_t kWindowGpsAgeNs = 1'000 * kMs;
constexpr uint64_t kReorderSlackNs = 2 * kMs;

uint16_t read_u16_le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
           (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

int32_t read_i32_le(const uint8_t* p) {
    return static_cast<int32_t>(read_u32_le(p));
}

uint64_t read_u64_le(const uint8_t* p) {
    return static_cast<uint64_t>(p[0]) |
           (static_cast<uint64_t>(p[1]) << 8) |
           (static_cast<uint64_t>(p[2]) << 16) |
           (static_cast<uint64_t>(p[3]) << 24) |
           (static_cast<uint64_t>(p[4]) << 32) |
           (static_cast<uint64_t>(p[5]) << 40) |
           (static_cast<uint64_t>(p[6]) << 48) |
           (static_cast<uint64_t>(p[7]) << 56);
}

uint32_t fnv1a32(const uint8_t* data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

struct CameraSample {
    uint64_t ts;
    uint32_t seq;
    int32_t frame;
};

struct ImuSample {
    uint64_t ts;
    uint32_t seq;
    int32_t yaw;
    int32_t pitch;
    int32_t roll;
};

struct GpsSample {
    uint64_t ts;
    uint32_t seq;
    int32_t lat;
    int32_t lon;
    int32_t alt;
};

struct ButtonEvent {
    uint64_t ts;
    uint32_t seq;
};

template <typename T>
bool sample_less(const T& a, const T& b) {
    if (a.ts != b.ts) {
        return a.ts < b.ts;
    }
    return a.seq < b.seq;
}

template <typename T>
void insert_sorted(std::deque<T>& container, const T& value) {
    auto it = std::lower_bound(container.begin(), container.end(), value, [](const T& lhs, const T& rhs) {
        return sample_less(lhs, rhs);
    });
    container.insert(it, value);
}

std::optional<CameraSample> pick_camera(const std::deque<CameraSample>& camera, uint64_t event_ts) {
    std::optional<CameraSample> best;
    uint64_t best_dist = std::numeric_limits<uint64_t>::max();

    for (const auto& s : camera) {
        uint64_t dist = (s.ts > event_ts) ? (s.ts - event_ts) : (event_ts - s.ts);
        if (dist > kWindowCameraNs) {
            continue;
        }
        if (!best.has_value() || dist < best_dist ||
            (dist == best_dist && (s.ts < best->ts || (s.ts == best->ts && s.seq < best->seq)))) {
            best = s;
            best_dist = dist;
        }
    }

    return best;
}

std::optional<ImuSample> pick_imu(const std::deque<ImuSample>& imu, uint64_t event_ts) {
    std::optional<ImuSample> best;
    uint64_t best_dist = std::numeric_limits<uint64_t>::max();

    for (const auto& s : imu) {
        uint64_t dist = (s.ts > event_ts) ? (s.ts - event_ts) : (event_ts - s.ts);
        if (dist > kWindowImuNearestNs) {
            continue;
        }
        if (!best.has_value() || dist < best_dist ||
            (dist == best_dist && (s.ts < best->ts || (s.ts == best->ts && s.seq < best->seq)))) {
            best = s;
            best_dist = dist;
        }
    }

    return best;
}

std::optional<GpsSample> pick_gps(const std::deque<GpsSample>& gps, uint64_t event_ts) {
    std::optional<GpsSample> best;

    for (const auto& s : gps) {
        if (s.ts > event_ts) {
            break;
        }
        if (event_ts - s.ts > kWindowGpsAgeNs) {
            continue;
        }
        best = s;
    }

    return best;
}

std::vector<ImuSample> pick_imu_window(const std::deque<ImuSample>& imu, uint64_t event_ts) {
    std::vector<ImuSample> window;
    const uint64_t lo = (event_ts > kWindowImuAroundNs) ? (event_ts - kWindowImuAroundNs) : 0;
    const uint64_t hi = event_ts + kWindowImuAroundNs;

    for (const auto& s : imu) {
        if (s.ts < lo) {
            continue;
        }
        if (s.ts > hi) {
            break;
        }
        window.push_back(s);
    }

    return window;
}

void emit_event(const ButtonEvent& event,
                const std::deque<CameraSample>& camera,
                const std::deque<ImuSample>& imu,
                const std::deque<GpsSample>& gps) {
    const auto camera_hit = pick_camera(camera, event.ts);
    const auto imu_hit = pick_imu(imu, event.ts);
    const auto gps_hit = pick_gps(gps, event.ts);
    const auto imu_window = pick_imu_window(imu, event.ts);

    std::cout << "{\"event_id\":" << event.seq << ",\"timestamp_ns\":" << event.ts << ",\"camera\":";
    if (camera_hit.has_value()) {
        std::cout << "{\"timestamp_ns\":" << camera_hit->ts << ",\"frame\":" << camera_hit->frame << "}";
    } else {
        std::cout << "null";
    }

    std::cout << ",\"imu\":";
    if (imu_hit.has_value()) {
        std::cout << "{\"timestamp_ns\":" << imu_hit->ts
                  << ",\"yaw_cd\":" << imu_hit->yaw
                  << ",\"pitch_cd\":" << imu_hit->pitch
                  << ",\"roll_cd\":" << imu_hit->roll << "}";
    } else {
        std::cout << "null";
    }

    std::cout << ",\"gps\":";
    if (gps_hit.has_value()) {
        std::cout << "{\"timestamp_ns\":" << gps_hit->ts
                  << ",\"lat_e7\":" << gps_hit->lat
                  << ",\"lon_e7\":" << gps_hit->lon
                  << ",\"alt_mm\":" << gps_hit->alt << "}";
    } else {
        std::cout << "null";
    }

    std::cout << ",\"imu_window\":[";
    for (size_t i = 0; i < imu_window.size(); ++i) {
        if (i > 0) {
            std::cout << ',';
        }
        const auto& s = imu_window[i];
        std::cout << "{\"timestamp_ns\":" << s.ts
                  << ",\"yaw_cd\":" << s.yaw
                  << ",\"pitch_cd\":" << s.pitch
                  << ",\"roll_cd\":" << s.roll << "}";
    }
    std::cout << "]}\n";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::array<uint8_t, 16> header{};
    std::cin.read(reinterpret_cast<char*>(header.data()), header.size());
    if (static_cast<size_t>(std::cin.gcount()) != header.size()) {
        std::cerr << "invalid header\n";
        return 1;
    }

    if (std::memcmp(header.data(), "USENS001", 8) != 0 ||
        read_u16_le(header.data() + 8) != 32 ||
        read_u16_le(header.data() + 10) != 1 ||
        read_u32_le(header.data() + 12) != fnv1a32(header.data(), 12)) {
        std::cerr << "invalid header\n";
        return 1;
    }

    std::deque<CameraSample> camera;
    std::deque<ImuSample> imu;
    std::deque<GpsSample> gps;
    std::deque<ButtonEvent> pending_buttons;

    uint64_t max_ts_seen = 0;

    auto prune_samples = [&]() {
        uint64_t retain_from = (max_ts_seen > (kWindowGpsAgeNs + kReorderSlackNs))
                                   ? (max_ts_seen - (kWindowGpsAgeNs + kReorderSlackNs))
                                   : 0;

        if (!pending_buttons.empty()) {
            uint64_t earliest_button = pending_buttons.front().ts;
            uint64_t pending_retain = (earliest_button > kWindowGpsAgeNs) ? (earliest_button - kWindowGpsAgeNs) : 0;
            if (pending_retain < retain_from) {
                retain_from = pending_retain;
            }
        }

        while (!camera.empty() && camera.front().ts < retain_from) {
            camera.pop_front();
        }
        while (!imu.empty() && imu.front().ts < retain_from) {
            imu.pop_front();
        }
        while (!gps.empty() && gps.front().ts < retain_from) {
            gps.pop_front();
        }
    };

    auto flush_ready_events = [&]() {
        while (!pending_buttons.empty()) {
            const auto& event = pending_buttons.front();
            if (max_ts_seen < event.ts + kWindowImuAroundNs + kReorderSlackNs) {
                break;
            }
            emit_event(event, camera, imu, gps);
            pending_buttons.pop_front();
        }
    };

    std::array<uint8_t, 32> record{};
    while (true) {
        std::cin.read(reinterpret_cast<char*>(record.data()), record.size());
        const std::streamsize got = std::cin.gcount();
        if (got == 0) {
            break;
        }
        if (static_cast<size_t>(got) != record.size()) {
            std::cerr << "truncated record\n";
            return 1;
        }

        if (read_u32_le(record.data() + 28) != fnv1a32(record.data(), 28)) {
            continue;
        }

        const uint64_t ts = read_u64_le(record.data());
        const uint32_t seq = read_u32_le(record.data() + 8);
        const uint8_t sensor_id = record[12];
        const uint8_t flags = record[13];
        const int32_t x = read_i32_le(record.data() + 16);
        const int32_t y = read_i32_le(record.data() + 20);
        const int32_t z = read_i32_le(record.data() + 24);

        if (ts > max_ts_seen) {
            max_ts_seen = ts;
        }

        if ((flags & 0x1u) == 0) {
            flush_ready_events();
            prune_samples();
            continue;
        }

        switch (sensor_id) {
            case 1:
                insert_sorted(camera, CameraSample{ts, seq, x});
                break;
            case 2:
                if (x >= 0 && x <= 35999) {
                    insert_sorted(imu, ImuSample{ts, seq, x, y, z});
                }
                break;
            case 3:
                insert_sorted(gps, GpsSample{ts, seq, x, y, z});
                break;
            case 5:
                if (x == 1) {
                    insert_sorted(pending_buttons, ButtonEvent{ts, seq});
                }
                break;
            default:
                break;
        }

        flush_ready_events();
        prune_samples();
    }

    while (!pending_buttons.empty()) {
        emit_event(pending_buttons.front(), camera, imu, gps);
        pending_buttons.pop_front();
    }

    return 0;
}
