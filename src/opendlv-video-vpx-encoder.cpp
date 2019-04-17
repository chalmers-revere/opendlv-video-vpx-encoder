/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         ( (0 == commandlineArguments.count("vp8")) && (0 == commandlineArguments.count("vp9")) ) ||
         ( (1 == commandlineArguments.count("vp8")) && (1 == commandlineArguments.count("vp9")) ) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding VPX (VP8 or VP9) frame for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--bitrate=<bitrate>] [--verbose] [--id=<identifier in case of multiple instances]" << std::endl;
        std::cerr << "         --vp8:     use VP8 encoder" << std::endl;
        std::cerr << "         --vp9:     use VP9 encoder" << std::endl;
        std::cerr << "         --cid:     CID of the OD4Session to send VP8 or VP9 frames" << std::endl;
        std::cerr << "         --id:      when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:    name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:   width of the frame" << std::endl;
        std::cerr << "         --height:  height of the frame" << std::endl;
        std::cerr << "         --gop:     optional: length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --bitrate: optional: desired bitrate (default: 800,000, min: 50,000 max: 5,000,000)" << std::endl;
        std::cerr << "         --verbose: print encoding information" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=data --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const bool VP8{commandlineArguments.count("vp8") != 0};
        const bool VP9{commandlineArguments.count("vp9") != 0};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t GOP_DEFAULT{10};
        const uint32_t GOP{(commandlineArguments["gop"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["gop"])) : GOP_DEFAULT};
        const uint32_t BITRATE_MIN{50000};
        const uint32_t BITRATE_DEFAULT{800000};
        const uint32_t BITRATE_MAX{5000000};
        const uint32_t BITRATE{(commandlineArguments["bitrate"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate"])), BITRATE_MIN), BITRATE_MAX) : BITRATE_DEFAULT};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};

        // Thesis addition.
        // const uint32_t PASSES{(commandlineArguments["passes"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["passes"])) : 1};
        // const uint32_t PASS{(commandlineArguments["pass"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["pass"])) : 1};
        //const uint32_t DEADLINE{(commandlineArguments["deadline"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["deadline"])) : 0};
        //const bool BEST{commandlineArguments.count("best") != 0};
        //const bool GOOD{commandlineArguments.count("good") != 0};
        //const bool RT{commandlineArguments.count("rt") != 0};
        //const uint32_t USAGE{(commandlineArguments["usage"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["usage"])) : 0};


        const uint32_t THREADS{(commandlineArguments["threads"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["threads"])) : 4};
        const uint32_t PROFILE{(commandlineArguments["profile"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["profile"])) : 0};
        const std::string STEREO_MODE{(commandlineArguments["stereo-mode"].size() != 0) ? commandlineArguments["stereo-mode"] : "mono"};
        //const uint32_t FPS{(commandlineArguments["fps"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["fps"])) : 30};
        const uint32_t LAG_IN_FRAMES{(commandlineArguments["lag-in-frames"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["lag-in-frames"])) : 0};
        const uint32_t DROP_FRAME{(commandlineArguments["drop-frame"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["drop-frame"])) : 0};
        const bool RESIZE_ALLOWED{commandlineArguments.count("resize-allowed") != 0};
        const uint32_t RESIZE_UP{(commandlineArguments["resize-up"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["resize-up"])) : 0};
        const uint32_t RESIZE_DOWN{(commandlineArguments["resize-down"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["resize-down"])) : 0};
        const uint32_t END_USAGE{(commandlineArguments["end-usage"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["end-usage"])) : 0};
        const uint32_t MIN_Q{(commandlineArguments["min-q"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["min-q"])) : 4};
        //const uint32_t MAX_Q{(commandlineArguments["max-q"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["max-q"])) : (VP8 ? 56 : 52)};
        const uint32_t UNDERSHOOT_PCT{(commandlineArguments["undershoot-pct"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["undershoot-pct"])) : 0};
        const uint32_t OVERSHOOT_PCT{(commandlineArguments["overshoot-pct"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["overshoot-pct"])) : 0};
        const uint32_t BUFFER_SIZE{(commandlineArguments["buffer-size"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["buffer-size"])) : 6000};
        const uint32_t BUFFER_INIT_SIZE{(commandlineArguments["buffer-init-size"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["buffer-init-size"])) : 4000};
        const uint32_t BUFFER_OPTIMAL_SIZE{(commandlineArguments["buffer-optimal-size"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["buffer-optimal-size"])) : 5000};
        const uint32_t KF_MODE{(commandlineArguments["kf-mode"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["kf-mode"])) : 0};
        const uint32_t KF_MIN_DIST{(commandlineArguments["kf-min-dist"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["kf-min-dist"])) : 0};
        const uint32_t KF_MAX_DIST{(commandlineArguments["kf-max-dist"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["kf-max-dist"])) : 99999};

        /*
            Skipped parameters: fpf, limit, skip, thread - Provides no value for our cause.

        */

        std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << "[opendlv-video-vpx-encoder]: Attached to '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

            vpx_codec_iface_t *encoderAlgorithm{(VP8 ? &vpx_codec_vp8_cx_algo : &vpx_codec_vp9_cx_algo)};

            vpx_image_t yuvFrame;
            if (!vpx_img_wrap(&yuvFrame, VPX_IMG_FMT_I420, WIDTH, HEIGHT, 1, reinterpret_cast<uint8_t*>(sharedMemory->data()))) {
                std::cerr << "[opendlv-video-vpx-encoder]: Failed to wrap shared memory into vpx_image." << std::endl;
                return retCode;
            }

            struct vpx_codec_enc_cfg parameters;
            memset(&parameters, 0, sizeof(parameters));
            vpx_codec_err_t result = vpx_codec_enc_config_default(encoderAlgorithm, &parameters, 0);
            if (result) {
                std::cerr << "[opendlv-video-vpx-encoder]: Failed to get default configuration: " << vpx_codec_err_to_string(result) << std::endl;
                return retCode;
            }

            parameters.rc_target_bitrate = BITRATE/1000;
            parameters.g_w = WIDTH;
            parameters.g_h = HEIGHT;
            parameters.g_timebase.num = 1;
            parameters.g_timebase.den = 20 /* implicitly given from notifyAll trigger*/;

            // Parameters according to https://www.webmproject.org/docs/encoder-parameters/
            parameters.g_threads = THREADS;
            parameters.rc_max_quantizer = (VP8 ? 56 : 52);



            // Thesis parameters.


            if (END_USAGE == 0) {
              parameters.rc_end_usage = VPX_CBR;
            } else {
              parameters.rc_end_usage = VPX_VBR;
            }



            parameters.g_profile = PROFILE;
            parameters.g_lag_in_frames = LAG_IN_FRAMES; // A value > 0 allows the encoder to consume more frames before emitting compressed frames.

            parameters.rc_dropframe_thresh = DROP_FRAME;
            parameters.rc_resize_allowed = RESIZE_ALLOWED;
            parameters.rc_resize_up_thresh = RESIZE_UP;
            parameters.rc_resize_down_thresh = RESIZE_DOWN;
            // Testing every q below rc_max_quantizer.
            parameters.rc_min_quantizer = MIN_Q;
            parameters.rc_undershoot_pct = UNDERSHOOT_PCT;
            parameters.rc_overshoot_pct = OVERSHOOT_PCT;

            parameters.rc_buf_sz = BUFFER_SIZE;
            parameters.rc_buf_initial_sz = BUFFER_INIT_SIZE;
            parameters.rc_buf_optimal_sz = BUFFER_OPTIMAL_SIZE;


            /*
                    IGNORED PARAMETERS:

              parameters.g_pass = PASS; //enum vpx_enc_pass
              parameters.rc_2pass_vbr_bias = 50;
              parameters.rc_2pass_vbr_minsection = 0;
              parameters.rc_2pass_vbr_maxsection = 0;

              In the documentation for the vpx encoder it is stated that
              2 pass encoding is impossible in the scope of real time streaming.

              Hence, no 2 pass encoding will be availible in this version.

            */

            // There is two keyframe modes, unsure of best way to handle this.


            if (KF_MODE == 1) {
              parameters.kf_mode = VPX_KF_DISABLED;
            } else {
              parameters.kf_mode = VPX_KF_AUTO;
            }

            parameters.kf_min_dist = KF_MIN_DIST;
            parameters.kf_max_dist = KF_MAX_DIST;

            vpx_codec_ctx_t codec;
            memset(&codec, 0, sizeof(codec));
            result = vpx_codec_enc_init(&codec, encoderAlgorithm, &parameters, 0);
            if (result) {
                std::cerr << "[opendlv-video-vpx-encoder]: Failed to initialize encoder: " << vpx_codec_err_to_string(result) << std::endl;
                return retCode;
            }
            else {
                std::clog << "[opendlv-video-vpx-encoder]: Using " << vpx_codec_iface_name(encoderAlgorithm) << std::endl;
            }
            vpx_codec_control(&codec, VP8E_SET_CPUUSED, 4);

            // Allocate image buffer to hold VP9 frame as output.
            std::vector<char> vpxBuffer;
            vpxBuffer.resize(WIDTH * HEIGHT, '0'); // In practice, this is smaller than WIDTH * HEIGHT

            uint32_t frameCounter{0};

            cluon::data::TimeStamp before, after, sampleTimeStamp;

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            while ( (sharedMemory && sharedMemory->valid()) && od4.isRunning() ) {
                // Wait for incoming frame.
                sharedMemory->wait();

                sampleTimeStamp = cluon::time::now();

                sharedMemory->lock();
                {
                    // Read notification timestamp.
                    auto r = sharedMemory->getTimeStamp();
                    sampleTimeStamp = (r.first ? r.second : sampleTimeStamp);
                }
                {
                    if (VERBOSE) {
                        before = cluon::time::now();
                    }
                    int flags{ (0 == (frameCounter%GOP)) ? VPX_EFLAG_FORCE_KF : 0 };
                    result = vpx_codec_encode(&codec, &yuvFrame, frameCounter, 1, flags, VPX_DL_REALTIME);
                    if (result) {
                        std::cerr << "[opendlv-video-vpx-encoder]: Failed to encode frame: " << vpx_codec_err_to_string(result) << std::endl;
                    }
                    if (VERBOSE) {
                        after = cluon::time::now();
                    }
                }
                sharedMemory->unlock();

                if (!result) {
                    vpx_codec_iter_t it{nullptr};
                    const vpx_codec_cx_pkt_t *packet{nullptr};

                    int totalSize{0};
                    while ((packet = vpx_codec_get_cx_data(&codec, &it))) {
                        switch (packet->kind) {
                            case VPX_CODEC_CX_FRAME_PKT:
                                memcpy(&vpxBuffer[totalSize], packet->data.frame.buf, packet->data.frame.sz);
                                totalSize += packet->data.frame.sz;
                            break;
                        default:
                            break;
                        }
                    }

                    if ( (0 < totalSize) && (VP8 || VP9) ) {
                        opendlv::proxy::ImageReading ir;
                        ir.fourcc((VP8 ? "VP80" : "VP90")).width(WIDTH).height(HEIGHT).data(std::string(&vpxBuffer[0], totalSize));
                        od4.send(ir, sampleTimeStamp, ID);

                        if (VERBOSE) {
                            std::clog << "[opendlv-video-vpx-encoder]: Frame size = " << totalSize << " bytes; sample time = " << cluon::time::toMicroseconds(sampleTimeStamp) << " microseconds; encoding took " << cluon::time::deltaInMicroseconds(after, before) << " microseconds." << std::endl;
                        }
                        frameCounter++;
                    }
                }
            }

            vpx_codec_destroy(&codec);

            retCode = 0;
        }
        else {
            std::cerr << "[opendlv-video-vpx-encoder]: Failed to attach to shared memory '" << NAME << "'." << std::endl;
        }
    }
    return retCode;
}
