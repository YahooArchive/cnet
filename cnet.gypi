# Copyright 2014, Yahoo! Inc.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'cnet_sources': [
      'cnet/cnet.cc',
      'cnet/cnet.h',
      'cnet/cnet_fetcher.cc',
      'cnet/cnet_fetcher.h',
      'cnet/cnet_headers.h',
      'cnet/cnet_mime.cc',
      'cnet/cnet_mime.h',
      'cnet/cnet_network_delegate.cc',
      'cnet/cnet_network_delegate.h',
      'cnet/cnet_oauth.cc',
      'cnet/cnet_oauth.h',
      'cnet/cnet_pool.cc',
      'cnet/cnet_pool.h',
      'cnet/cnet_proxy_service.cc',
      'cnet/cnet_proxy_service.h',
      'cnet/cnet_response.cc',
      'cnet/cnet_response.h',
      'cnet/cnet_url_params.h',
    ],
    'cnet_android_sources': [
      'cnet/android/cnet_jni.cc',
      'cnet/android/cnet_jni.h',
      'cnet/android/cnet_jni_registrar.cc',
      'cnet/android/cnet_jni_registrar.h',
      'cnet/android/cnet_adapter.cc',
      'cnet/android/cnet_adapter.h',
      'cnet/android/fetcher_adapter.cc',
      'cnet/android/fetcher_adapter.h',
      'cnet/android/pool_adapter.cc',
      'cnet/android/pool_adapter.h',
      'cnet/android/response_adapter.cc',
      'cnet/android/response_adapter.h',
      'cnet/android/response_completion_adapter.cc',
      'cnet/android/response_completion_adapter.h',
    ],
  },
  'targets': [
    {
      'target_name': 'libcnet',
      'type': 'shared_library',
      'ldflags': [
        '-Wl,--gc-sections',
        '-Wl,--exclude-libs,ALL',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/net/net.gyp:net',
      ],
      'sources': [ '<@(cnet_sources)', ],
      'conditions': [
        [ 'use_icu_alternatives_on_android == 1', {
            'dependencies!': [
              '<(DEPTH)/base/base.gyp:base_i18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
            ]
        }],
        [ 'OS=="android"', {
            'sources': [
              '<@(cnet_android_sources)',
            ],
            'dependencies': [
              'cnet_jni_headers',
              'cnet_java',
            ],
        }],
      ],
    },
    {
      'target_name': 'libcnet_internal',
      'type': 'shared_library',
      'ldflags': [
        '-Wl,--gc-sections',
        '-Wl,--exclude-libs,ALL',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/net/net.gyp:net',
      ],
      'sources': [ '<@(cnet_sources)', ],
      'conditions': [
        [ 'use_icu_alternatives_on_android == 1', {
            'dependencies!': [
              '<(DEPTH)/base/base.gyp:base_i18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
            ]
        }],
        [ 'OS=="android"', {
            'sources': [
              '<@(cnet_android_sources)',
            ],
            'dependencies': [
              'cnet_jni_headers',
              'cnet_java',
            ],
        }],
      ],
    },
    {
      'target_name': 'cnet-util',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/net/net.gyp:net',
      ],
      'sources': [
        '<@(cnet_sources)',
        'cnet_util.cc',
      ],
      'conditions': [
        [ 'use_icu_alternatives_on_android == 1', {
            'dependencies!': [
              '<(DEPTH)/base/base.gyp:base_i18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
              '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
            ]
        }],
      ],
    },
    {
      'target_name': 'cnet_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/net/net.gyp:net_test_support',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
      'sources': [
        '<@(cnet_sources)',
        'cnet_unittest.cc',
        '<(DEPTH)/net/test/net_test_suite.cc',
        '<(DEPTH)/net/test/net_test_suite.h',
      ],
    },
  ],
  'conditions': [
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'cnet_jni_headers',
          'type': 'none',
          'sources': [
            'android/java/src/com/yahoo/cnet/Cnet.java',
            'android/java/src/com/yahoo/cnet/CnetFetcher.java',
            'android/java/src/com/yahoo/cnet/CnetPool.java',
            'android/java/src/com/yahoo/cnet/CnetResponse.java',
            'android/java/src/com/yahoo/cnet/ResponseCompletion.java',
          ],
          'variables': {
            'jni_gen_package': 'cnet',
          },
          'includes': [ '../../build/jni_generator.gypi' ],
        },
        {
          'target_name': 'cnet_java',
          'type': 'none',
          'dependencies': [
            '<(DEPTH)/base/base.gyp:base',
          ],
          'variables': {
            'java_in_dir': 'android/java',
            'javac_includes': [
              '**/cnet/*.java',
            ],
          },
          'includes': [ '../../build/java.gypi' ],
        },
        {
          'target_name': 'cnet_package',
          'type': 'none',
          'dependencies': [ 'libcnet' ],
          'variables': {
            'native_lib': 'libcnet.>(android_product_extension)',
            'java_lib': 'cnet.jar',
            'java_src_lib': 'cnet-src.jar',
            'package_dir': '<(PRODUCT_DIR)/cnet',
            'java_srcs_dir': '<(package_dir)/java/src',
            'intermediate_dir': '<(SHARED_INTERMEDIATE_DIR)/cnet',
            'jar_extract_dir': '<(intermediate_dir)/cnet_jar_extract',
            'jar_excluded_classes': [
              '*/library_loader/*',
            ],
            'jar_extract_stamp': '<(intermediate_dir)/jar_extract.stamp',
            'cnet_jar_stamp': '<(intermediate_dir)/cnet_jar.stamp',
          },
          'actions': [
             {
               'action_name': 'strip libcnet',
               'inputs': ['<(SHARED_LIB_DIR)/<(native_lib)'],
               'outputs': ['<(package_dir)/libs/<(android_app_abi)/<(native_lib)'],
               'action': [
                 '<(android_strip)',
                 '--strip-unneeded',
                 '<@(_inputs)',
                 '-o',
                 '<@(_outputs)',
               ],
             },
             {
               'action_name': 'extracting from jars',
               'inputs': [
                 '<(PRODUCT_DIR)/lib.java/cnet_java.jar',
                 '<(PRODUCT_DIR)/lib.java/base_java.jar',
                 '<(PRODUCT_DIR)/lib.java/net_java.jar',
                 '<(PRODUCT_DIR)/lib.java/url_java.jar',
               ],
               'outputs': ['<(jar_extract_stamp)', '<(jar_extract_dir)'],
               'action': [
                 'python',
                 '<(DEPTH)/components/cronet/tools/extract_from_jars.py',
                 '--classes-dir=<(jar_extract_dir)',
                 '--jars=<@(_inputs)',
                 '--stamp=<(jar_extract_stamp)',
               ],
             },
             {
               'action_name': 'jar_<(_target_name)',
               'message': 'Creating <(_target_name) jar',
               'inputs': [
                 '<(DEPTH)/build/android/gyp/util/build_utils.py',
                 '<(DEPTH)/build/android/gyp/util/md5_check.py',
                 '<(DEPTH)/build/android/gyp/jar.py',
                 '<(jar_extract_stamp)',
               ],
               'outputs': [
                 '<(package_dir)/<(java_lib)',
                 '<(cnet_jar_stamp)',
               ],
               'action': [
                 'python', '<(DEPTH)/build/android/gyp/jar.py',
                 '--classes-dir=<(jar_extract_dir)',
                 '--jar-path=<(package_dir)/<(java_lib)',
                 '--excluded-classes=<@(jar_excluded_classes)',
                 '--stamp=<(cnet_jar_stamp)',
               ],
             },
             {
               'action_name': 'jar_src_<(_target_name)',
               'inputs': [
                 'tools/jar_src.py',
                 '<(jar_extract_stamp)',
               ],
               'outputs': ['<(package_dir)/<(java_src_lib)'],
               'action': [
                 'python',
                 '<(DEPTH)/yahoo/cnet/tools/jar_src.py',
                 '--target-path=<(java_srcs_dir)',
                 '--src-path=<(DEPTH)/yahoo/cnet/android/java/src/com/yahoo/cnet',
                 '--src-path=<(DEPTH)/base/android/java/src',
                 '--src-path=<(DEPTH)/net/android/java/src',
                 '--src-path=<(DEPTH)/url/android/java/src',
                 '--src-path=<(DEPTH)/third_party/jsr-305/src/ri/src/main/java/',
                 '--gen-path=<(PRODUCT_DIR)/gen/enums/certificate_mime_types_java/org/chromium/net,org/chromium/net',
                 '--gen-path=<(PRODUCT_DIR)/gen/enums/cert_verify_status_android_java/org/chromium/net,org/chromium/net',
                 '--gen-path=<(PRODUCT_DIR)/gen/enums/private_key_types_java/org/chromium/net,org/chromium/net',
                 '--gen-path=<(PRODUCT_DIR)/gen/remote_android_keystore_aidl/aidl,org/chromium/net',
                 '--gen-path=<(PRODUCT_DIR)/gen/templates/base_java_application_state/org/chromium/base,org/chromium/base',
                 '--gen-path=<(PRODUCT_DIR)/gen/templates/base_java_memory_pressure_level_list/org/chromium/base,org/chromium/base',
                 '--gen-path=<(PRODUCT_DIR)/gen/templates/net_errors_java/org/chromium/net,org/chromium/net',
                 '--jar-path=<(package_dir)/<(java_src_lib)',
               ],
             },
          ],
          'copies': [
            {
              'destination': '<(package_dir)',
              'files': [
                '<(DEPTH)/chrome/VERSION',
                '<(DEPTH)/components/cronet/android/proguard.cfg',
              ],
            },
          ],
        },
      ],
    }],
  ],
}
