Cnet
===============

Cnet is an easy-to-use wrapper around the high-performance Chromium HTTP
network stack and file cache.  It includes Chromium's SPDY support and
experimental QUIC support.

Cnet has language bindings for C and Java.  See `android/java` for the 
complete Java API.  See [`cnet.h`](cnet.h) for the complete C API.  An
application can use the Java and C bindings simultaneously.

## Example Java Fetch

```java
// Initialize the library.
final Context appContext = getApplicationContext();
Cnet.loadLibraries(appContext);
Cnet.initLibraryOnUiThread(appContext);

// Establish a Cnet Pool for connection reuse.
final CnetPool.Config poolConfig = new CnetPool.Config();
poolConfig.userAgent = "cnet-test";
poolConfig.enableSpdy = true;
poolConfig.enableSslFalseStart = true;
final CnetPool pool = new CnetPool(poolConfig);

// Create an HTTP request.
final Fetcher fetcher = pool.createFetcher("https://yahoo.com", "GET", new ResponseCompletion() {
    @Override
    public boolean onBackgroundComplete(Fetcher fetcher, Response response) {
        Log.d(LOG_TAG, "request completed, " + response.getHttpResponseCode() + 
              ", " + response.getOriginalUrl() + ", " + response.getFinalUrl() +
              ", " + response.getBodyLength() + ", " +
              (new String(response.getBody())));
        return RELEASE_NOW; // immediately release native resources
   }
});
fetcher.start();
```

Note that I've released the native resources used by the response and
fetcher (by returning `RELEASE_NOW` from the `onBackgroundComplete()`
callback).  This ensures immediate recovery of native memory, instead
of waiting for the Java garbage collector that is unaware of the amount of
memory consumed by the native representation.  This is really important for
the response object, which might be large.  If you wanted to keep the
response data for use at a later time, then return `RELEASE_LATER`,
and manually release its resources by invoking `response.release()`.

## Library Initialization

For Java, you should load the libraries using
`Cnet.loadLibraries(Context)`.  This works around dynamic linking bugs in
several versions of Android.

For Java, initialize the library on the main UI thread
(`Cnet.initLibraryOnUiThread(Context)`), so that
it can grab a reference to the UI thread's dispatch loop.  Complete the
initialization before interacting with Cnet on background threads.

For the C API, while on the UI thread, grab a reference to the UI's dispatch
loop via `CnetMessageLoopForUiGet()`, and initialize the library
(`CnetInitialize(false)`).  Complete the initialization before interacting
with Cnet on background threads.

## Pools

A pool provides shared resources for a sequence of HTTP requests, such as
reused sockets and a file cache.  Avoid enabling the file cache for pools
dedicated to API requests.

Each pool can create three threads:

1. a networking thread that runs all network logic;
2. a work thread that runs all callbacks (so that callbacks do not
   block the networking thread), one at a time;
3. and a file thread (created on demand), for reading files for uploads
   and writing files for downloads.

You can adjust several settings on pools:
* SSL false start: enable this to reduce SSL-connection times by 1/3.
* Proxy config: by default, Cnet uses the system's proxy settings (e.g.,
  on Android, it registers a broadcast listener for proxy changes), but
  you may override these for debugging with Charles Proxy.  The
  override expects a rule similar to `http://10.73.218.92:8888`.
* Trust self-signed certificate authorities: by default, and always in
  release builds, Cnet will reject self-signed certificate
  authorities.  When debugging with Charles Proxy, you'll probably
  want to enable self-signed certificate authorities.

## Fetching

The fetcher is a simple abstraction that, upon completion, provides a
buffer or a file with the entire response --- it doesn't use a streaming
abstraction.  Thus the fetcher is convenient for JSON and images,
which typically we want only when completely downloaded.  It always
provides the response on a background thread.

To stop a fetcher, you must invoke its cancel method --- trying to
delete the fetcher will not stop its execution (it retains a reference
to itself, to provide deterministic behavior in garbage-collected
environments).  A fetcher always executes its completion callback,
even when cancelled.

The fetcher makes it easy to submit request parameters, whether as
part of the query in the URL, or encoded as a multi-part form.  It
complements this with Oauth v1 support and file-upload support.

When creating a fetcher, you provide the following required parameters:
* the URL,
* the HTTP method (`GET`, `POST`, `PUT`, etc.),
* and the response/completion callback.

After you have created a fetcher, you can customize some of its behavior:
* Cache behavior: normal, validate, bypass, prefer, cache only, cache if
  offline, and cache disable (see the source files for documentation of each).
* Parameter encoding: the query string for the URL, multi-part body, or a query 
  string in the body.
* Set parameters.
* Set the upload file.
* Set the Oauth v1 credentials.

## The Fetcher Response

The response includes access to the timing (telemetry) data of different stages
of the HTTP request.  On Android, this data is printed to the log, where it
can be extracted and converted to a HAR file.

The response includes the body of the response as an array of bytes.  On
Android, you can get the raw pointer to this (as a long), which you can pass
to another native library (such as Ymagine to decode an image).

# Related Work

The Chromium project already has a component,
[_cronet_](https://chromium.googlesource.com/chromium/src/+/master/components/cronet/),
which packages the Chromium network stack for use in Android applications.
Our Cnet wrapper gives us the ability to expose the Chromium features that
we need.  Alternatively, we could have modified Cronet, but then we'd
amplify the task of merging our code with the evolving Chromium code.

# Design

## API

The fetcher simplifies the task of requesting composites such as JSON
and images.  It provides the response as a byte-array --- not a
streaming API.  It is not a general-purpose API to HTTP and all of its
features --- it is a simplification for use by common mobile
applications.  When we need more features, we'll add a second request
type that offers more generalized features.

## Threading

Cnet generally supports multi-threading via message passing: one
thread can own the Cnet objects at a time, but can transfer ownership
of the objects to other threads.  We try for additional safety to be
defensive:
* The response object is immutable, and thus is safely accessed
  concurrently from multiple threads.
* Any thread can change a Cnet pool, for the pool will commit the changes
  on the network thread via message passing.

# cnet-util Command-Line Utility

Example Flickr fetch of the interesting list, using Charles Proxy:
```
./out/Debug/cnet-util --method=POST --encoding=body-multipart \
     --proxy-rules=http://10.73.218.92:8888 \
     --oauth-app-key=${MY_OAUTH_APP_KEY} \
     --oauth-app-secret=${MY_OAUTH_APP_SECRET} \
     --oauth-token=${MY_OAUTH_TOKEN} \
     --oauth-token-secret=${MY_OAUTH_TOKEN_SECRET} \
     method=flickr.favorites.getList \
     page=1 per_page=10  format=json nojsoncallback=1 \
     api_key=${MY_FLICKR_API_KEY}  --trust-all-cert-authorities \
     https://api.flickr.com/services/rest
```

Example Flickr upload, using Charles Proxy:
```
./out/Debug/cnet-util --method=POST --encoding=body-multipart  \
    --proxy-rules=http://10.73.218.92:8888 \
    --oauth-app-key=${MY_OAUTH_APP_KEY} \
    --oauth-app-secret=${MY_OAUTH_APP_SECRET} \
    --oauth-token=${MY_OAUTH_TOKEN} \
    --oauth-token-secret=${MY_OAUTH_TOKEN_SECRET} \
    --upload=${HOME}/Pictures/test-photo.jpg --upload-key=photo \
    --upload-type=image/jpeg "title=testing upload" name=test.jpg \
    is_public=0 is_friend=0 is_family=0 async=0 \
    api_key=${MY_FLICKR_API_KEY}  --trust-all-cert-authorities \
    https://up.flickr.com/services/upload
```

# Building Cnet from Source

## Initial Checkout and Build

The Cnet code must be built within the Chromium environment.  Android
requires a Linux build machine.

1. For Android, review the system requirements for your build machine at
   <https://code.google.com/p/chromium/wiki/AndroidBuildInstructions>, but
   don't follow its checkout instructions.  For iOS, review the system requirements for
   your build machine at <http://www.chromium.org/developers/how-tos/build-instructions-ios>,
   but don't follow its checkout instructions.
   In particular, you need Google's `depot_tools` somewhere in your path.
2. Create a top-level directory such as `chromium`.
3. Within your top-level directory, create a file called `.gclient` with the following contents:

   ```
solutions = [{
  u'managed': False,
  u'name': u'src',
  u'url': u'https://chromium.googlesource.com/chromium/src.git',
  u'custom_deps': {
    "src/yahoo/cnet":"https://github.com/yahoo/cnet.git",
  },
  u'custom_hooks': [
    {
      "name": "cnet-gyp",
      "pattern": ".",
      "action": ["python", "src/build/gyp_chromium", "src/yahoo/cnet/cnet.gyp"],
    },
  ],
  u'deps_file': u'.DEPS.git',
}]
target_os = ['ios', 'mac']
   ```
   If using Android, change `target_os` to `['android']`.
4. Within your top-level directory, create a file called `chromium.gyp_env`.
   For Android, the file should have:

   ```
{
  'GYP_DEFINES': 'OS=android enable_websockets=0 use_icu_alternatives_on_android=1 disable_ftp_support=1 disable_file_support=1',
}
   ```
   You have to choose the architecture by adding additional variables: `target_arch=arm arm_version=6`, or
   `target_arch=arm arm_version=7`, or `target_arch=ia32`.

   For iOS, the file should have:

   ```
{
  'GYP_DEFINES': 'OS=ios', # use space to delimit additional defines.
  'GYP_GENERATOR_FLAGS': 'xcode_project_version=3.2',
  'GYP_GENERATORS': 'ninja,xcode',
}
   ```

   For Mac, the file should have:

   ```
{
  'GYP_DEFINES': 'OS=mac', # use space to delimit additional defines.
  'GYP_GENERATOR_FLAGS': 'xcode_project_version=3.2',
  'GYP_GENERATORS': 'ninja,xcode',
}
   ```

5. Within your top-level directory, checkout the release code:
   run `gclient sync --with_branch_heads --jobs 16`.
   This may fail with an error about duplicate IDs, which you can ignore.
6. Within the `src` directory, determine the newest Chromium release
   version: run `git show-ref --tags` and find the newest, e.g., 39.0.2130.1.
7. Within the `src` directory, checkout the latest release version:
   `git checkout -b rel-39.0.2130.1 39.0.2130.1`
8. Re-sync to update all dependencies to the versions that correspond
   to the latest Chromium release:
   run `gclient sync --with_branch_heads --jobs 16`
9. If the `gclient sync` command failed, then you'll have to manually generate
   the build files: in the source directory,
   run `./build/gyp_chromium yahoo/cnet/cnet.gyp`.
10. If you are developing for iOS or Mac, you can then open `src/yahoo/cnet/cnet.xcodeproj`.
11. To build the cnet-util, from the command line, run: `ninja -C out/Debug cnet-util`.
    Or to build the Android cnet package, run: `ninja -C out/Debug cnet_package`.

## Building for Android

You probably want to support multiple Android architectures, such as
`armeabi` (arm6), `armeabi-v7a` (arm7), and `x86` (ia32).  This
requires maintaining multiple, concurrent Chromium build trees.

The following instructions will create three build trees with output
in `out/arm6/out/Debug`, `out/arm7/out/Debug`, and
`out/ia32/out/Debug`:

1. Comment out the `GYP_DEFINES` in the `chromium.gyp_env` file in
   your Chromium root directory.  You're going to specify them on the
   command line instead.
2. Generate the build files (in the top-level `src` directory):

   ```
GYP_DEFINES='target_arch=arm arm_version=7 OS=android enable_websockets=0 use_icu_alternatives_on_android=1 disable_ftp_support=1 disable_file_support=1' \
GYP_GENERATOR_OUTPUT='out/arm7' ./build/gyp_chromium yahoo/cnet/cnet.gyp
GYP_DEFINES='target_arch=arm arm_version=6 OS=android enable_websockets=0 use_icu_alternatives_on_android=1 disable_ftp_support=1 disable_file_support=1' \
GYP_GENERATOR_OUTPUT='out/arm6' ./build/gyp_chromium yahoo/cnet/cnet.gyp
GYP_DEFINES='target_arch=ia32 OS=android enable_websockets=0 use_icu_alternatives_on_android=1 disable_ftp_support=1 disable_file_support=1' \
GYP_GENERATOR_OUTPUT='out/ia32' ./build/gyp_chromium yahoo/cnet/cnet.gyp
   ```

3. Build for each architecture:

   ```
   make -C out/arm7/out/Debug cnet_package
   make -C out/arm6/out/Debug cnet_package
   make -C out/ia32/out/Debug cnet_package
   ```

4. The `cnet_package` generates a variety of outputs: `libcnet.so`,
   `cnet.jar`, `cnet-src.jar`, and `java/*`; all under the `cnet`
   subdirectory of the output directory.  Only the `libcnet.so` files
   should be architecture dependent.  You can consume the Java
   via a compiled jar, a source jar, or the raw source files
   themselves (under `java/*`).  You may need the symbols for the dynamic
   link library: they are in the output directory as `lib/libcnet.so`.


