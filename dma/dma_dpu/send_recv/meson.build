project('InformedRackSched', 'C', 'CPP',
	version: run_command(find_program('cat'),
		files('/opt/mellanox/doca/applications/VERSION'), check: true).stdout().strip(),
	license: 'Proprietary',
	default_options: ['buildtype=debug'],
	meson_version: '>= 0.61.2'
)

# Integrate DPDK as an External Dependency:
dpdk_dep = dependency('libdpdk', method: 'pkg-config')

# Integrate DOCA using pkg-config
doca_dep = dependency('doca')

NAME = 'informed_rack_sched'

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c', 'cpp'])

app_dependencies = []
app_dependencies += dependency('doca')
app_dependencies += dpdk_dep

app_srcs=[
	NAME + '.c',
	NAME + '_main.c',
	# Common code for the DOCA DMA
	'../dma_common.c',
	# Common code for all DOCA Code
	'../../common.c',
	# Common code for all DOCA applications
	'/opt/mellanox/doca/applications/common/src/utils.c',
	'/opt/mellanox/doca/applications/common/src/doca_utils.c',
]

app_srcs = [
  'dma_common.c', 'dma_tx.c', 'doca_common.c', 'main.c',
  'port_init.c', 'recv_pkt.c', 'send_pkt.c',
# Common code for all DOCA applications
'/opt/mellanox/doca/applications/common/src/utils.c',
'/opt/mellanox/doca/applications/common/src/doca_utils.c',
]

inc_dirs= []
inc_dirs += include_directories('..')
inc_dirs += include_directories('../..')
inc_dirs += include_directories('/opt/mellanox/doca/')
inc_dirs += include_directories('/opt/mellanox/doca/applications/common/src')
inc_dirs += include_directories('/opt/mellanox/doca/lib')
inc_dirs += include_directories('/usr/local/include')



executable(NAME, app_srcs,
	c_args : ['-DALLOW_EXPERIMENTAL_API', '-Wno-missing-braces'],
	dependencies : app_dependencies,
	include_directories: inc_dirs,
	install: false)