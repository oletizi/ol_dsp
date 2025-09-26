import type { ArdourMidiMap, ArdourBinding, ArdourDeviceInfo } from '@/types/ardour.js';

/**
 * Configuration options for XML serialization formatting.
 * Controls the output format of generated Ardour MIDI map XML.
 *
 * @example
 * ```typescript
 * const options: XMLSerializerOptions = {
 *   indent: '    ', // 4 spaces
 *   newline: '\r\n' // Windows line endings
 * };
 * ```
 */
export interface XMLSerializerOptions {
  /** String used for indentation (defaults to 2 spaces) */
  indent?: string;
  /** String used for line endings (defaults to \n) */
  newline?: string;
}

/**
 * Serializes and parses Ardour MIDI map configurations to/from XML format.
 *
 * This class handles the conversion between TypeScript objects and Ardour's XML format,
 * including proper XML escaping, formatting, and validation.
 *
 * Key features:
 * - Bidirectional conversion (serialize to XML, parse from XML)
 * - Proper XML escaping for special characters
 * - Organized output with comments for binding groups
 * - MIDI protocol validation (channels 1-16, CC/note numbers 0-127)
 * - Support for all Ardour binding types (CC, Note, RPN, NRPN, etc.)
 *
 * @example
 * ```typescript
 * const serializer = new ArdourXMLSerializer({ indent: '  ' });
 *
 * // Serialize a MIDI map to XML
 * const xml = serializer.serializeMidiMap(midiMap);
 *
 * // Parse XML back to MIDI map
 * const parsedMap = serializer.parseMidiMap(xml);
 * ```
 */
export class ArdourXMLSerializer {
  private readonly indent: string;
  private readonly newline: string;

  /**
   * Creates a new XML serializer with the specified formatting options.
   *
   * @param options - Formatting configuration for XML output
   *
   * @example
   * ```typescript
   * // Default formatting (2 spaces, Unix line endings)
   * const serializer = new ArdourXMLSerializer();
   *
   * // Custom formatting
   * const customSerializer = new ArdourXMLSerializer({
   *   indent: '\t',    // Tab indentation
   *   newline: '\r\n'  // Windows line endings
   * });
   * ```
   */
  constructor(options: XMLSerializerOptions = {}) {
    this.indent = options.indent ?? '  ';
    this.newline = options.newline ?? '\n';
  }

  /**
   * Serializes an Ardour MIDI map to XML format.
   *
   * Generates a complete Ardour MIDI bindings XML file including:
   * - XML declaration
   * - Root ArdourMIDIBindings element with name and version
   * - Optional DeviceInfo element
   * - Organized binding groups with comments
   * - Individual bindings with proper MIDI parameter validation
   *
   * @param midiMap - The MIDI map to serialize
   * @returns Well-formed XML string ready for Ardour
   *
   * @throws Error if the MIDI map contains invalid data
   *
   * @example
   * ```typescript
   * const midiMap: ArdourMidiMap = {
   *   name: 'My Controller',
   *   version: '1.0.0',
   *   bindings: [
   *     { channel: 1, ctl: 7, function: 'track-set-gain[1]' }
   *   ]
   * };
   *
   * const xml = serializer.serializeMidiMap(midiMap);
   * // Result: <?xml version="1.0" encoding="UTF-8"?>
   * // <ArdourMIDIBindings version="1.0.0" name="My Controller">
   * //   <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
   * // </ArdourMIDIBindings>
   * ```
   */
  serializeMidiMap(midiMap: ArdourMidiMap): string {
    const version = midiMap.version || '1.0.0';
    const lines: string[] = [
      '<?xml version="1.0" encoding="UTF-8"?>',
      `<ArdourMIDIBindings version="${version}" name="${this.escapeXML(midiMap.name)}">`,
    ];

    // Add DeviceInfo if present
    if (midiMap.deviceInfo) {
      lines.push(this.serializeDeviceInfoElement(midiMap.deviceInfo));
    }

    // Serialize binding groups with comments if available
    if (midiMap.bindingGroups && midiMap.bindingGroups.length > 0) {
      for (const group of midiMap.bindingGroups) {
        // Add comment for the group
        if (group.comment) {
          lines.push(this.indent + `<!-- ${this.escapeXML(group.comment)} -->`);
        }

        // Add all bindings in this group
        for (const binding of group.bindings) {
          lines.push(this.indent + this.serializeBinding(binding));
        }

        // Add empty line between groups for readability
        if (group !== midiMap.bindingGroups[midiMap.bindingGroups.length - 1]) {
          lines.push('');
        }
      }
    }

    // Add any ungrouped bindings
    for (const binding of midiMap.bindings.filter(binding =>
      !midiMap.bindingGroups?.some(group => group.bindings.includes(binding))
    )) {
      lines.push(this.indent + this.serializeBinding(binding));
    }

    lines.push('</ArdourMIDIBindings>');
    return lines.join(this.newline);
  }

  /**
   * Serializes device information to standalone XML format.
   * Useful for creating separate device info files or debugging.
   *
   * @param deviceInfo - Device capabilities and information
   * @returns XML string with device information
   *
   * @example
   * ```typescript
   * const deviceInfo: ArdourDeviceInfo = {
   *   'device-name': 'My Controller',
   *   'device-info': {
   *     'bank-size': 8,
   *     'motorized': 'no',
   *     'has-lcd': 'yes'
   *   }
   * };
   *
   * const xml = serializer.serializeDeviceInfo(deviceInfo);
   * ```
   */
  serializeDeviceInfo(deviceInfo: ArdourDeviceInfo): string {
    const lines: string[] = [
      '<?xml version="1.0" encoding="UTF-8"?>',
      `<DeviceInfo name="${this.escapeXML(deviceInfo['device-name'])}">`,
    ];

    const info = deviceInfo['device-info'];
    const attributes: string[] = [];

    for (const [key, value] of Object.entries(info)) {
      if (value !== undefined) {
        attributes.push(`${key}="${value}"`);
      }
    }

    if (attributes.length > 0) {
      lines.push(`${this.indent}<GlobalButtons ${attributes.join(' ')}/>`);
    }

    lines.push('</DeviceInfo>');
    return lines.join(this.newline);
  }

  /**
   * Serializes device info as an inline XML element for inclusion in MIDI maps.
   * @private
   */
  private serializeDeviceInfoElement(deviceInfo: ArdourDeviceInfo): string {
    const info = deviceInfo['device-info'];
    const attributes: string[] = [];

    for (const [key, value] of Object.entries(info)) {
      if (value !== undefined) {
        attributes.push(`${key}="${value}"`);
      }
    }

    return `<DeviceInfo ${attributes.join(' ')}/>`;
  }

  /**
   * Serializes a single MIDI binding to XML format.
   * Handles all MIDI message types and validates parameter ranges.
   * @private
   */
  private serializeBinding(binding: ArdourBinding): string {
    const attributes: string[] = [];

    attributes.push(`channel="${binding.channel}"`);

    if (binding.ctl !== undefined) {
      attributes.push(`ctl="${binding.ctl}"`);
    }
    if (binding.note !== undefined) {
      attributes.push(`note="${binding.note}"`);
    }
    if (binding['enc-r'] !== undefined) {
      attributes.push(`enc-r="${binding['enc-r']}"`);
    }
    if (binding.rpn !== undefined) {
      attributes.push(`rpn="${binding.rpn}"`);
    }
    if (binding.nrpn !== undefined) {
      attributes.push(`nrpn="${binding.nrpn}"`);
    }
    if (binding.rpn14 !== undefined) {
      attributes.push(`rpn-14="${binding.rpn14}"`);
    }
    if (binding.nrpn14 !== undefined) {
      attributes.push(`nrpn-14="${binding.nrpn14}"`);
    }

    if (binding.function) {
      attributes.push(`function="${this.escapeXML(binding.function)}"`);
    }
    if (binding.uri) {
      attributes.push(`uri="${this.escapeXML(binding.uri)}"`);
    }

    if (binding.action) {
      attributes.push(`action="${this.escapeXML(binding.action)}"`);
    }
    if (binding.encoder) {
      attributes.push(`encoder="${binding.encoder}"`);
    }
    if (binding.momentary) {
      attributes.push(`momentary="${binding.momentary}"`);
    }
    if (binding.threshold !== undefined) {
      attributes.push(`threshold="${binding.threshold}"`);
    }

    return `<Binding ${attributes.join(' ')}/>`;
  }

  /**
   * Escapes special XML characters in text content.
   * Converts: & < > " ' to their XML entity equivalents.
   * @private
   */
  private escapeXML(text: string): string {
    return text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&apos;');
  }

  /**
   * Unescapes XML entities back to their original characters.
   * Converts: &amp; &lt; &gt; &quot; &apos; back to & < > " '
   * @private
   */
  private unescapeXML(text: string): string {
    return text
      .replace(/&amp;/g, '&')
      .replace(/&lt;/g, '<')
      .replace(/&gt;/g, '>')
      .replace(/&quot;/g, '"')
      .replace(/&apos;/g, "'");
  }

  /**
   * Parses an Ardour MIDI map from XML format.
   *
   * Validates and converts XML back to TypeScript objects with:
   * - MIDI channel validation (1-16)
   * - MIDI CC/note number validation (0-127)
   * - RPN/NRPN parameter validation (0-16383)
   * - Boolean attribute parsing ('yes'/'no' to boolean flags)
   * - XML unescaping for special characters
   *
   * @param xml - XML string to parse
   * @returns Parsed MIDI map object
   *
   * @throws Error if XML is malformed or contains invalid MIDI data
   *
   * @example
   * ```typescript
   * const xml = `<?xml version="1.0" encoding="UTF-8"?>
   * <ArdourMIDIBindings version="1.0.0" name="Test Map">
   *   <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
   * </ArdourMIDIBindings>`;
   *
   * const map = serializer.parseMidiMap(xml);
   * console.log(map.name); // "Test Map"
   * console.log(map.bindings[0].ctl); // 7
   * ```
   */
  parseMidiMap(xml: string): ArdourMidiMap {
    if (!xml || typeof xml !== 'string') {
      throw new Error('Invalid XML input: expected non-empty string');
    }

    // Remove XML declaration and normalize whitespace
    const cleanXml = xml.trim().replace(/<?xml[^>]*?>\s*/, '');

    // Parse the root ArdourMIDIBindings element
    const rootMatch = cleanXml.match(/<ArdourMIDIBindings\s+([^>]*)>/i);
    if (!rootMatch) {
      throw new Error('Invalid XML: missing ArdourMIDIBindings root element');
    }

    // Extract name and version from root attributes
    const rootAttributes = this.parseAttributes(rootMatch[1] || '');
    const name = rootAttributes['name'];
    if (!name) {
      throw new Error('Invalid XML: ArdourMIDIBindings element must have a name attribute');
    }

    const version = rootAttributes['version'];

    // Find all Binding elements
    const bindingMatches = cleanXml.match(/<Binding\s+([^>]*)\/>/gi) || [];
    const bindings: ArdourBinding[] = [];

    for (const bindingMatch of bindingMatches) {
      const attributeString = bindingMatch.match(/<Binding\s+([^>]*)\/>/i)?.[1];
      if (!attributeString) continue;

      const attributes = this.parseAttributes(attributeString);
      try {
        const binding = this.parseBinding(attributes);
        bindings.push(binding);
      } catch (error) {
        throw new Error(`Error parsing binding: ${(error as Error).message}`);
      }
    }

    // Parse DeviceInfo if present
    let deviceInfo: ArdourDeviceInfo | undefined;
    const deviceInfoMatch = cleanXml.match(/<DeviceInfo\s+([^>]*)\/>/i);
    if (deviceInfoMatch && deviceInfoMatch[1]) {
      const deviceAttributes = this.parseAttributes(deviceInfoMatch[1]);
      deviceInfo = this.parseDeviceInfo(deviceAttributes);
    }

    const result: ArdourMidiMap = {
      name,
      bindings
    };

    if (version !== undefined) {
      result.version = version;
    }

    if (deviceInfo !== undefined) {
      result.deviceInfo = deviceInfo;
    }

    return result;
  }

  /**
   * Parses XML attribute string into key-value pairs.
   * Handles both single and double quotes, with proper XML unescaping.
   * @private
   */
  private parseAttributes(attributeString: string): Record<string, string> {
    const attributes: Record<string, string> = {};

    // Match attribute="value" or attribute='value' patterns
    const attrRegex = /(\w+(?:-\w+)*)\s*=\s*["']([^"']*)["']/g;
    let match;

    while ((match = attrRegex.exec(attributeString)) !== null) {
      if (match[1] && match[2] !== undefined) {
        attributes[match[1]] = this.unescapeXML(match[2]);
      }
    }

    return attributes;
  }

  /**
   * Parses XML attributes into an ArdourBinding object.
   * Validates all MIDI parameters and converts string attributes to appropriate types.
   *
   * @private
   * @param attributes - Parsed XML attributes
   * @returns Validated ArdourBinding object
   * @throws Error for invalid MIDI parameters (channels, CC numbers, etc.)
   */
  private parseBinding(attributes: Record<string, string>): ArdourBinding {
    const channelStr = attributes['channel'];
    if (!channelStr) {
      throw new Error('Binding element must have a channel attribute');
    }
    const channel = parseInt(channelStr, 10);
    if (isNaN(channel) || channel < 1 || channel > 16) {
      throw new Error(`Invalid MIDI channel: ${channelStr}. Must be 1-16.`);
    }

    const binding: ArdourBinding = { channel };

    // Parse MIDI message type attributes
    if (attributes['ctl'] !== undefined) {
      const ctl = parseInt(attributes['ctl'], 10);
      if (isNaN(ctl) || ctl < 0 || ctl > 127) {
        throw new Error(`Invalid MIDI CC: ${attributes['ctl']}. Must be 0-127.`);
      }
      binding.ctl = ctl;
    }

    if (attributes['note'] !== undefined) {
      const note = parseInt(attributes['note'], 10);
      if (isNaN(note) || note < 0 || note > 127) {
        throw new Error(`Invalid MIDI note: ${attributes['note']}. Must be 0-127.`);
      }
      binding.note = note;
    }

    if (attributes['enc-r'] !== undefined) {
      const encR = parseInt(attributes['enc-r'], 10);
      if (isNaN(encR) || encR < 0 || encR > 127) {
        throw new Error(`Invalid enc-r value: ${attributes['enc-r']}. Must be 0-127.`);
      }
      binding['enc-r'] = encR;
    }

    if (attributes['rpn'] !== undefined) {
      const rpn = parseInt(attributes['rpn'], 10);
      if (isNaN(rpn) || rpn < 0 || rpn > 16383) {
        throw new Error(`Invalid RPN: ${attributes['rpn']}. Must be 0-16383.`);
      }
      binding.rpn = rpn;
    }

    if (attributes['nrpn'] !== undefined) {
      const nrpn = parseInt(attributes['nrpn'], 10);
      if (isNaN(nrpn) || nrpn < 0 || nrpn > 16383) {
        throw new Error(`Invalid NRPN: ${attributes['nrpn']}. Must be 0-16383.`);
      }
      binding.nrpn = nrpn;
    }

    if (attributes['rpn-14'] !== undefined) {
      const rpn14 = parseInt(attributes['rpn-14'], 10);
      if (isNaN(rpn14) || rpn14 < 0 || rpn14 > 16383) {
        throw new Error(`Invalid RPN-14: ${attributes['rpn-14']}. Must be 0-16383.`);
      }
      binding.rpn14 = rpn14;
    }

    if (attributes['nrpn-14'] !== undefined) {
      const nrpn14 = parseInt(attributes['nrpn-14'], 10);
      if (isNaN(nrpn14) || nrpn14 < 0 || nrpn14 > 16383) {
        throw new Error(`Invalid NRPN-14: ${attributes['nrpn-14']}. Must be 0-16383.`);
      }
      binding.nrpn14 = nrpn14;
    }

    // Parse function and action attributes
    if (attributes['function']) {
      binding.function = attributes['function'];
    }

    if (attributes['uri']) {
      binding.uri = attributes['uri'];
    }

    if (attributes['action']) {
      binding.action = attributes['action'];
    }

    // Parse boolean attributes
    if (attributes['encoder'] !== undefined) {
      if (attributes['encoder'] === 'yes' || attributes['encoder'] === 'no') {
        binding.encoder = attributes['encoder'];
      } else {
        throw new Error(`Invalid encoder value: ${attributes['encoder']}. Must be 'yes' or 'no'.`);
      }
    }

    if (attributes['momentary'] !== undefined) {
      if (attributes['momentary'] === 'yes' || attributes['momentary'] === 'no') {
        binding.momentary = attributes['momentary'];
      } else {
        throw new Error(`Invalid momentary value: ${attributes['momentary']}. Must be 'yes' or 'no'.`);
      }
    }

    // Parse threshold
    if (attributes['threshold'] !== undefined) {
      const threshold = parseInt(attributes['threshold'], 10);
      if (isNaN(threshold) || threshold < 0 || threshold > 127) {
        throw new Error(`Invalid threshold: ${attributes['threshold']}. Must be 0-127.`);
      }
      binding.threshold = threshold;
    }

    return binding;
  }

  /**
   * Parses device info attributes into ArdourDeviceInfo object.
   * Validates boolean attributes and numeric values.
   *
   * @private
   * @param attributes - Parsed XML attributes
   * @returns Device information object with default device name
   */
  private parseDeviceInfo(attributes: Record<string, string>): ArdourDeviceInfo {
    // DeviceInfo elements in MIDI bindings don't have a name - they contain device properties
    // The device name is implicit based on the parent MIDI map
    const deviceInfo: ArdourDeviceInfo['device-info'] = {};

    // Parse all known device info attributes
    if (attributes['bank-size'] !== undefined) {
      const bankSize = parseInt(attributes['bank-size'], 10);
      if (!isNaN(bankSize) && bankSize > 0) {
        deviceInfo['bank-size'] = bankSize;
      }
    }

    const booleanAttrs: Array<keyof ArdourDeviceInfo['device-info']> = [
      'motorized', 'has-master-fader', 'has-lcd', 'has-timecode', 'has-meters',
      'uses-logic-control-buttons', 'uses-mackie-control-buttons', 'uses-ipmidi',
      'has-touch-sense-faders', 'has-jog-wheel', 'has-global-controls', 'has-segmented-display'
    ];

    for (const attr of booleanAttrs) {
      const value = attributes[attr];
      if (value !== undefined) {
        if (value === 'yes' || value === 'no') {
          (deviceInfo as any)[attr] = value;
        }
      }
    }

    if (attributes['threshold'] !== undefined) {
      const threshold = parseInt(attributes['threshold'], 10);
      if (!isNaN(threshold) && threshold >= 0 && threshold <= 127) {
        deviceInfo.threshold = threshold;
      }
    }

    return {
      'device-name': 'Default Device', // Use a default name since it's not in the XML
      'device-info': deviceInfo
    };
  }
}