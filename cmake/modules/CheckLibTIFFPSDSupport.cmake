# SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
# SPDX-License-Identifier: BSD-3-Clause

# Test libtiff support for Photoshop TIFFs
#
# This module defines
#   TIFF_HAS_PSD_TAGS -- if TIFFTAG_IMAGESOURCEDATA and TIFFTAG_PHOTOSHOP are defined in tiff.h
#   TIFF_CAN_WRITE_PSD_TAGS -- if TIFFTAG_IMAGESOURCEDATA and TIFFTAG_PHOTOSHOP can be written by a call to TiffSetField

function(check_libtiff_psd_support varname_read_tags varname_write_tags)
    include(CheckCXXSourceCompiles)
    include(CheckCXXSourceRuns)

    set(CMAKE_REQUIRED_LIBRARIES TIFF::TIFF)
    if (APPLE)
        set(CMAKE_REQUIRED_FLAGS "-rpath ${CMAKE_INSTALL_PREFIX}/lib")
    endif()
    check_cxx_source_compiles("
        #include <cstdint>
        #include <tiffio.h>
        int main()
        {
            TIFF *img;
            uint32_t length; uint8_t *data;
            TIFFGetField(img, TIFFTAG_IMAGESOURCEDATA, &length, &data);
            TIFFGetField(img, TIFFTAG_PHOTOSHOP, &length, &data);
        }
    " ${varname_read_tags})
    set(_krita_tiff_write_psd_tags_source "
        #include <array>
        #include <cstdio>
        #include <cstdint>
        #include <tiff.h>
        #include <tiffio.h>
        int main() {
            TIFF *img = TIFFOpen(\"test.tif\", \"w\");
            TIFFCreateDirectory(img);
            const std::array<uint8_t, 4> data = {0, 0, 0, 0};
            if (!TIFFSetField(img, TIFFTAG_PHOTOSHOP,
                            static_cast<uint32_t>(data.size()), data.data())) {
                TIFFClose(img);
                std::remove(\"test.tif\");
                return -1;
            }
            if (!TIFFSetField(img, TIFFTAG_IMAGESOURCEDATA,
                            static_cast<uint32_t>(data.size()), data.data())) {
                TIFFClose(img);
                std::remove(\"test.tif\");
                return -1;
            }
            TIFFClose(img);
            std::remove(\"test.tif\");
            return 0;
        }
    ")

    if(CMAKE_CROSSCOMPILING)
        # Target programs cannot run while configuring a cross build.  Keep
        # the API/link check here; runtime semantics are validated on-device.
        check_cxx_source_compiles(
            "${_krita_tiff_write_psd_tags_source}"
            ${varname_write_tags}
        )
    else()
        check_cxx_source_runs(
            "${_krita_tiff_write_psd_tags_source}"
            ${varname_write_tags}
        )
    endif()
endfunction(check_libtiff_psd_support)

find_package(TIFF REQUIRED QUIET)
if (TIFF_FOUND)
    if (NOT DEFINED TIFF_HAS_PSD_TAGS OR NOT DEFINED TIFF_CAN_WRITE_PSD_TAGS)
        check_libtiff_psd_support(TIFF_HAS_PSD_TAGS TIFF_CAN_WRITE_PSD_TAGS)
    endif()
    if (NOT TIFF_HAS_PSD_TAGS OR NOT TIFF_CAN_WRITE_PSD_TAGS)
        message(WARNING "Your version of libtiff cannot read or write Photoshop TIFFs!")
    endif (NOT TIFF_HAS_PSD_TAGS OR NOT TIFF_CAN_WRITE_PSD_TAGS)
endif(TIFF_FOUND)
