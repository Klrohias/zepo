export function generate(buildReport, exportNames, packageName) {
    const exportNameSelf = exportNames[packageName];

    if (buildReport['type'] === 'header-only') {
        let includeDirs = buildReport['paths']['includes']
            .map(x => `list(APPEND ${exportNameSelf}_INCLUDE_DIRS [=[${x}]=])`)
            .join('\n');

        return `
#
# Generated by Zepo Package Manager
#

add_library(${exportNameSelf} INTERFACE)

set(${exportNameSelf}_INCLUDE_DIRS)
${includeDirs}

target_include_directories (${exportNameSelf} INTERFACE $\{${exportNameSelf}_INCLUDE_DIRS\})

        `;
    }

    return `message (FATAL_ERROR "Unsupported type ${buildReport['type']}")`;
}