module.exports = [
  {
    "type": "heading",
    "defaultValue": "NEOCC Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Configure the remote control"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Connection"
      },
      {
        "type": "input",
        "messageKey": "host",
        "defaultValue": "http://asterix:5061/controlcenter/dotrigger",
        "label": "End point"
      },
      {
        "type": "input",
        "messageKey": "token",
        "defaultValue": "",
        "label": "Security token"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];