// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Architecture',
      collapsed: false,
      items: [
        'architecture/overview',
        'architecture/file-map',
        'architecture/main-loop',
      ],
    },
    {
      type: 'category',
      label: 'Core concepts',
      collapsed: false,
      items: [
        'core/modes',
        'core/data-model',
        'core/channels',
      ],
    },
    {
      type: 'category',
      label: 'Audio',
      items: [
        'audio/graph',
        'audio/samples',
        'audio/synths',
        'audio/effects',
      ],
    },
    {
      type: 'category',
      label: 'Subsystems',
      items: [
        'subsystems/ui-leds',
        'subsystems/midi-clock',
        'subsystems/menu',
        'subsystems/sd-storage',
      ],
    },
    {
      type: 'category',
      label: 'Tools',
      collapsed: false,
      items: [
        'tools/overview',
        'tools/sample-converter',
        'tools/midi-convert',
        'tools/sd-tool',
        'tools/firmware-loader',
        'tools/color-scheme',
        'tools/annotate',
      ],
    },
    {
      type: 'category',
      label: 'Hardware (rev G)',
      collapsed: false,
      items: [
        'hardware/overview',
        'hardware/connectors',
        'hardware/power',
        'hardware/firmware-pins',
        'hardware/fabrication',
      ],
    },
    {
      type: 'category',
      label: 'Contributing',
      items: [
        'contributing/build-setup',
        'contributing/memory',
        'contributing/docs-site',
      ],
    },
  ],
};

export default sidebars;
