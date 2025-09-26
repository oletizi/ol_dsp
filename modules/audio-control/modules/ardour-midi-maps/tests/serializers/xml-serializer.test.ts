/**
 * Tests for XML serialization and parsing
 */

import { describe, it, expect, beforeEach } from 'vitest';
import { ArdourXMLSerializer } from '@/serializers/xml-serializer.js';
import type { ArdourMidiMap, ArdourBinding, ArdourDeviceInfo } from '@/types/ardour.js';

describe('ArdourXMLSerializer', () => {
  let serializer: ArdourXMLSerializer;

  beforeEach(() => {
    serializer = new ArdourXMLSerializer();
  });

  describe('serializeMidiMap', () => {
    it('should serialize basic MIDI map', () => {
      const midiMap: ArdourMidiMap = {
        name: 'Test Controller',
        version: '1.0.0',
        bindings: [
          {
            channel: 1,
            ctl: 7,
            function: 'track-set-gain[1]'
          },
          {
            channel: 1,
            note: 36,
            function: 'toggle-track-mute[1]',
            momentary: 'yes'
          }
        ]
      };

      const xml = serializer.serializeMidiMap(midiMap);
      expect(xml).toContain('<?xml version="1.0" encoding="UTF-8"?>');
      expect(xml).toContain('<ArdourMIDIBindings version="1.0.0" name="Test Controller">');
      expect(xml).toContain('<Binding channel="1" ctl="7" function="track-set-gain[1]"/>');
      expect(xml).toContain('<Binding channel="1" note="36" function="toggle-track-mute[1]" momentary="yes"/>');
      expect(xml).toContain('</ArdourMIDIBindings>');
    });

    it('should handle special characters in names', () => {
      const midiMap: ArdourMidiMap = {
        name: 'Test & "Controller" <2>',
        bindings: []
      };

      const xml = serializer.serializeMidiMap(midiMap);
      expect(xml).toContain('name="Test &amp; &quot;Controller&quot; &lt;2&gt;"');
    });
  });

  describe('parseMidiMap', () => {
    it('should parse basic XML to MIDI map', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Test Controller">
  <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
  <Binding channel="1" note="36" function="toggle-track-mute[1]" momentary="yes"/>
</ArdourMIDIBindings>`;

      const midiMap = serializer.parseMidiMap(xml);

      expect(midiMap.name).toBe('Test Controller');
      expect(midiMap.version).toBe('1.0.0');
      expect(midiMap.bindings).toHaveLength(2);

      expect(midiMap.bindings[0]).toEqual({
        channel: 1,
        ctl: 7,
        function: 'track-set-gain[1]'
      });

      expect(midiMap.bindings[1]).toEqual({
        channel: 1,
        note: 36,
        function: 'toggle-track-mute[1]',
        momentary: 'yes'
      });
    });

    it('should parse XML with device info', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Test Controller">
  <DeviceInfo bank-size="8" motorized="yes" has-lcd="no"/>
  <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      const midiMap = serializer.parseMidiMap(xml);

      expect(midiMap.deviceInfo).toBeDefined();
      expect(midiMap.deviceInfo!['device-name']).toBe('Default Device');
      expect(midiMap.deviceInfo!['device-info']['bank-size']).toBe(8);
      expect(midiMap.deviceInfo!['device-info'].motorized).toBe('yes');
      expect(midiMap.deviceInfo!['device-info']['has-lcd']).toBe('no');
    });

    it('should parse all binding attribute types', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Test Controller">
  <Binding channel="2" ctl="1" function="track-set-gain[1]" encoder="yes"/>
  <Binding channel="3" note="64" action="custom-action" threshold="80"/>
  <Binding channel="4" enc-r="32" uri="http://example.com/plugin"/>
  <Binding channel="5" rpn="100" function="transport-start"/>
  <Binding channel="6" nrpn="200" momentary="no"/>
  <Binding channel="7" rpn-14="1000" function="track-set-pan[1]"/>
  <Binding channel="8" nrpn-14="2000" encoder="no"/>
</ArdourMIDIBindings>`;

      const midiMap = serializer.parseMidiMap(xml);

      expect(midiMap.bindings).toHaveLength(7);

      // Check each binding type
      expect(midiMap.bindings[0]).toEqual({
        channel: 2,
        ctl: 1,
        function: 'track-set-gain[1]',
        encoder: 'yes'
      });

      expect(midiMap.bindings[1]).toEqual({
        channel: 3,
        note: 64,
        action: 'custom-action',
        threshold: 80
      });

      expect(midiMap.bindings[2]).toEqual({
        channel: 4,
        'enc-r': 32,
        uri: 'http://example.com/plugin'
      });

      expect(midiMap.bindings[3]).toEqual({
        channel: 5,
        rpn: 100,
        function: 'transport-start'
      });

      expect(midiMap.bindings[4]).toEqual({
        channel: 6,
        nrpn: 200,
        momentary: 'no'
      });

      expect(midiMap.bindings[5]).toEqual({
        channel: 7,
        rpn14: 1000,
        function: 'track-set-pan[1]'
      });

      expect(midiMap.bindings[6]).toEqual({
        channel: 8,
        nrpn14: 2000,
        encoder: 'no'
      });
    });

    it('should handle XML without version attribute', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test Controller">
  <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      const midiMap = serializer.parseMidiMap(xml);

      expect(midiMap.name).toBe('Test Controller');
      expect(midiMap.version).toBeUndefined();
      expect(midiMap.bindings).toHaveLength(1);
    });

    it('should handle escaped characters in attributes', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test &amp; &quot;Controller&quot; &lt;2&gt;">
  <Binding channel="1" ctl="7" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      const midiMap = serializer.parseMidiMap(xml);
      expect(midiMap.name).toBe('Test & "Controller" <2>');
    });
  });

  describe('XML parsing error handling', () => {
    it('should throw error for invalid XML input', () => {
      expect(() => serializer.parseMidiMap('')).toThrow('Invalid XML input');
      expect(() => serializer.parseMidiMap(null as any)).toThrow('Invalid XML input');
      expect(() => serializer.parseMidiMap(123 as any)).toThrow('Invalid XML input');
    });

    it('should throw error for missing root element', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<InvalidRoot name="test">
</InvalidRoot>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('missing ArdourMIDIBindings root element');
    });

    it('should throw error for missing name attribute', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0">
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('must have a name attribute');
    });

    it('should throw error for invalid MIDI channel', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="0" ctl="7" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid MIDI channel: 0. Must be 1-16.');
    });

    it('should throw error for invalid MIDI CC', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="1" ctl="128" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid MIDI CC: 128. Must be 0-127.');
    });

    it('should throw error for invalid MIDI note', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="1" note="-1" function="toggle-mute"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid MIDI note: -1. Must be 0-127.');
    });

    it('should throw error for invalid boolean attributes', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="1" ctl="7" encoder="maybe" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid encoder value: maybe. Must be \'yes\' or \'no\'.');
    });

    it('should throw error for invalid RPN values', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="1" rpn="16384" function="track-set-gain[1]"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid RPN: 16384. Must be 0-16383.');
    });

    it('should throw error for invalid threshold', () => {
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings name="Test">
  <Binding channel="1" note="60" threshold="200" function="toggle-mute"/>
</ArdourMIDIBindings>`;

      expect(() => serializer.parseMidiMap(xml)).toThrow('Invalid threshold: 200. Must be 0-127.');
    });
  });

  describe('round-trip serialization', () => {
    it('should maintain data integrity through serialize -> parse -> serialize', () => {
      const originalMap: ArdourMidiMap = {
        name: 'Round Trip Test',
        version: '2.0.0',
        bindings: [
          {
            channel: 1,
            ctl: 7,
            function: 'track-set-gain[1]',
            encoder: 'yes'
          },
          {
            channel: 2,
            note: 64,
            action: 'custom-action',
            momentary: 'no',
            threshold: 100
          },
          {
            channel: 3,
            rpn: 500,
            uri: 'http://example.com/plugin'
          }
        ],
        deviceInfo: {
          'device-name': 'Test Device',
          'device-info': {
            'bank-size': 16,
            'motorized': 'yes',
            'has-lcd': 'no',
            'threshold': 64
          }
        }
      };

      // Serialize to XML
      const xml = serializer.serializeMidiMap(originalMap);

      // Parse back to object
      const parsedMap = serializer.parseMidiMap(xml);

      // Serialize again
      const xml2 = serializer.serializeMidiMap(parsedMap);

      // Both XML outputs should be functionally equivalent
      // (though formatting may differ slightly)
      const parsedMap2 = serializer.parseMidiMap(xml2);

      expect(parsedMap2.name).toBe(originalMap.name);
      expect(parsedMap2.version).toBe(originalMap.version);
      expect(parsedMap2.bindings).toHaveLength(originalMap.bindings.length);
      expect(parsedMap2.deviceInfo).toBeDefined();
      expect(parsedMap2.deviceInfo!['device-info']).toEqual(originalMap.deviceInfo!['device-info']);

      // Check all bindings match
      for (let i = 0; i < originalMap.bindings.length; i++) {
        expect(parsedMap2.bindings[i]).toEqual(originalMap.bindings[i]);
      }
    });
  });
});