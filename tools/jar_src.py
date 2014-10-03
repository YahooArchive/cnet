#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Changes to this code are Copyright 2014 Yahoo! Inc.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import fnmatch
import optparse
import os
import shutil
import sys

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(REPOSITORY_ROOT, 'build/android/gyp/util'))
import build_utils

def FindSources(src_dir, files):
  for root, _, filenames in os.walk(src_dir):
    for f in fnmatch.filter(filenames, "*.java"):
      origin_src = os.path.join(root, f)
      relative_src = os.path.relpath(origin_src, src_dir)
      files[origin_src] = relative_src

def FindGenSources(gen_dir, java_path, files):
  for root, _, filenames in os.walk(gen_dir):
    for f in fnmatch.filter(filenames, "*.java"):
      origin_src = os.path.join(root, f)
      relative_src = os.path.join(java_path, os.path.relpath(origin_src, gen_dir))
      files[origin_src] = relative_src

def InstallFiles(target_dir, files):
  try:
    os.mkdir(target_dir)
  except OSError as e:
    pass

  for src in files:
    dst_file = os.path.join(target_dir, files[src])

    dst_dir = os.path.dirname(dst_file)
    try:
      os.makedirs(dst_dir)
    except OSError as e:
      if e.errno == errno.EEXIST and os.path.isdir(dst_dir):
        pass
      else:
        raise

    shutil.copy2(src, dst_file)


def JarSources(src_dir, jar_path):
  # The paths of the files in the jar will be the same as they are passed in to
  # the command. Because of this, the command should be run in
  # options.src_dir so the .java file paths in the jar are correct.
  jar_path = os.path.abspath(jar_path)
  jar_cmd = ['jar', 'cf', jar_path, '-C', src_dir, '.']
  build_utils.CheckOutput(jar_cmd)


def main():
  parser = optparse.OptionParser()
  parser.add_option('--src-path', action="append", help='Directory containing .java files.')
  parser.add_option('--gen-path', action="append", help='Directory containing generated .java files, separated by a comma from the target path (e.g., org/chromium/net).')
  parser.add_option('--target-path', help='Directory for installing all Java files.')
  parser.add_option('--jar-path', help='Jar output path.')
  parser.add_option('--stamp', help='Path to touch on success.')

  options, _ = parser.parse_args()

  files = {}
  if options.src_path:
    for src_path in options.src_path:
      FindSources(src_path, files)
  if options.gen_path:
    for gen_pair in options.gen_path:
      gen_path, rel_path = gen_pair.split(",")
      FindGenSources(gen_path, rel_path, files)

  if options.target_path:
    InstallFiles(options.target_path, files)
    if options.jar_path:
      JarSources(options.target_path, options.jar_path)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  sys.exit(main())

