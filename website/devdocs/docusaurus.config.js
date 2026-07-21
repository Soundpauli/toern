// @ts-check
import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'TŒRN Code Docs',
  tagline: 'How the firmware fits together',
  favicon: 'img/favicon.svg',

  future: {
    v4: true,
  },

  // Served from the main site at https://toern.live/docs/
  url: 'https://toern.live',
  baseUrl: '/docs/',

  organizationName: 'Soundpauli',
  projectName: 'toern',

  onBrokenLinks: 'throw',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          routeBasePath: '/',
          sidebarPath: './sidebars.js',
          editUrl: 'https://github.com/Soundpauli/toern/tree/main/website/devdocs/',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      image: 'img/docusaurus-social-card.jpg',
      colorMode: {
        defaultMode: 'dark',
        respectPrefersColorScheme: true,
      },
      navbar: {
        title: 'TŒRN Code',
        logo: {
          alt: 'TŒRN',
          src: 'img/favicon.svg',
        },
        items: [
          {
            type: 'docSidebar',
            sidebarId: 'docsSidebar',
            position: 'left',
            label: 'Docs',
          },
          {
            href: 'https://github.com/Soundpauli/toern',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Code docs',
            items: [
              {label: 'Architecture', to: '/architecture/overview'},
              {label: 'File map', to: '/architecture/file-map'},
              {label: 'This docs site', to: '/contributing/docs-site'},
            ],
          },
          {
            title: 'Hardware',
            items: [
              {label: 'rev G overview', to: '/hardware/overview'},
              {label: 'Connectors', to: '/hardware/connectors'},
              {label: 'Fabrication', to: '/hardware/fabrication'},
            ],
          },
          {
            title: 'Elsewhere',
            items: [
              {
                label: 'Operator handbook',
                href: 'https://toern.live/handbook/',
              },
              {
                label: 'Main site',
                href: 'https://toern.live/',
              },
              {
                label: 'PCB sources',
                href: 'https://github.com/Soundpauli/toern/tree/main/PCB/toern_revG',
              },
              {
                label: 'Source repo',
                href: 'https://github.com/Soundpauli/toern',
              },
            ],
          },
        ],
        copyright: `TŒRN code & hardware docs · MIT software · CC BY-NC hardware · Built with Docusaurus`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['cpp', 'bash', 'json'],
      },
    }),
};

export default config;
