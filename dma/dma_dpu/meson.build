project('InformedRackSched', 'C', 'CPP',
	version: run_command(find_program('cat'),
		files('/opt/mellanox/doca/applications/VERSION'), check: true).stdout().strip(),
	license: 'Proprietary',
	default_options: ['buildtype=debug'],
	meson_version: '>= 0.61.2'
)

NAME = 'dma_copy_dpu'

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c', 'cpp'])

app_dependencies = []
app_dependencies += dependency('doca')

app_srcs=[
    # The code itself
	NAME + '.c',
	# Main function for the sample's executable
	NAME + '_main.c',
	# Common code for the DOCA DMA
	'../dma_common.c',
	# Common code for all DOCA Code
	'../../common.c',
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


executable('doca_' + NAME, app_srcs,
	c_args : '-Wno-missing-braces',
	dependencies : app_dependencies,
	include_directories: inc_dirs,
	install: false)


