project('rproxy', ['c', 'cpp'],
	default_options: [
		'c_std=c11'
    ],
    license: 'MIT',
    version: '0.1.1'
)

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form [C, R, A].
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to [C+1, 0, A+1].
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to [C+1, 0, 0]
# - If the interface is the same as the previous version, use [C, R+1, A].
rproxycore_soversion = [12, 0, 0]

# Mangle [C, R, A] into an actual usable *soversion*.
rproxycore_soversion_major = rproxycore_soversion[0] - rproxycore_soversion[2]  # Current-Age
rproxycore_soversion_minor = rproxycore_soversion[2]  # Age
rproxycore_soversion_micro = rproxycore_soversion[1]  # Revision
rproxycore_soversion = '@0@.@1@.@2@'.format(rproxycore_soversion_major, rproxycore_soversion_minor, rproxycore_soversion_micro)

project_version_components = meson.project_version().split('.')

glib_dep = dependency('glib-2.0', required: true)
openssl_dep = dependency('openssl')
libevent_dep = dependency('libevent', required: true)
libevent_core_dep = dependency('libevent_core', required: true)
libevent_openssl_dep = dependency('libevent_openssl', required: true)
libevent_pthreads_dep = dependency('libevent_pthreads', required: true)
libevent_extra_dep = dependency('libevent_extra', required: true)
libevhtp_dep = dependency('evhtp', required: true)
confuse_dep = dependency('libconfuse')
brotlicommon_dep = dependency('libbrotlicommon')
brotlidec_dep = dependency('libbrotlidec')
brotlienc_dep = dependency('libbrotlienc')

#if get_option('documentation')
#    subdir('docs')
#endif

#if get_option('examples')
#    subdir('examples')
#endif

subdir('src')
