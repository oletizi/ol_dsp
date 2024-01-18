# Daisy Synth App

## Performance

| Git ID                                   | CPU Load                          | Change                                         |
|------------------------------------------|-----------------------------------|------------------------------------------------|
| 9da43346c98b8461dcaef50142237613efedf289 | CPU: min: 76%; max: 76%; avg: 76% | Baseline                                       |
| 8fc7544870c18a63a827e4f225d4449a1cd1531b | CPU: min: 66%; max: 67%; avg: 67% | Switched to MoogLadder from Svf in SynthVoice. | 
| db159ff750b41f134759fecfdff7e0f2110ab7a6 | CPU: min: 66%; max: 67%; avg: 67% | Detemplatized Polyvoice.                       |
| 6f154fe2452ff5303aa251275ae80de872a20084 | CPU: min: 73%; max: 74%; avg: 74% | Added delay to ol_daisy/app/synth              |
| 2bfe815c2af9b02461cae248fe539c01553e2e36 | CPU: min: 86%; max: 87%; avg: 86% | Added reverb                                   |
| 506f2f3802105c25fe3855d453a0aba032786de5 | CPU: min: 52%; max: 53%; avg: 52% | Turned off debug in Makefile                   |