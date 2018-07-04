'use strict';
let data = `
120611532000 00 81 00 a7 18 a0 03 20
120612132000 00 a0 00 a7 18 a0 03 20
120612583000 00 81 00 a7 18 a0 03 20
120613152000 00 a0 00 a7 18 a0 03 20
120613662000 00 81 00 a7 18 a0 03 20
120614202000 00 a0 00 a7 18 a0 03 20
120614682000 00 81 00 a7 18 a0 03 20
120615253000 00 a0 00 a7 18 a0 03 20
120615462000 20 81 20 00 00 00 00 00
120616062000 00 a0 20 1f 1f e5 00 00
120616273000 20 a1 00 47 18 00 35 00
120616512000 21 a2 00 47 18 20 35 00
120617022000 00 a0 00 a7 18 a0 03 20
120617232000 20 a1 10 00 00 00 00 00
120617442000 21 81 10 00 00 00 00 00
120618043000 00 a0 10 01 00 00 00 00
120618252000 20 a1 00 27 18 00 35 00
120618462000 21 81 00 27 18 20 35 00
120619092000 00 a0 00 a7 18 a0 03 20
120619302000 20 a1 00 27 18 00 35 00
120619512000 21 81 00 27 18 20 35 00
120620022000 00 a0 00 a7 18 a0 03 20
120620232000 20 a1 00 27 18 00 35 00
120620442000 21 81 00 27 18 20 35 00
120620952000 00 a0 00 a7 18 a0 03 20
120621162000 20 a1 00 27 18 00 35 00
120621372000 21 81 00 27 18 20 35 00
120621852000 00 a0 00 a7 18 a0 03 20
120622062000 20 a1 00 27 18 00 35 00
120622302000 21 81 00 27 18 20 35 00
120622782000 00 a0 00 a7 18 a0 03 20
120623022000 20 a1 00 27 18 00 35 00
120623232000 21 81 00 27 18 20 35 00
120623833000 00 a0 00 a7 18 a0 03 20
120624073000 20 a1 00 27 18 00 35 00
120624282000 21 81 00 27 18 20 35 00
120624882000 00 a0 00 a7 18 a0 03 20
120625092000 20 a1 00 27 18 00 35 00
120625302000 21 81 00 27 18 20 35 00
120625902000 00 a0 00 a7 18 a0 03 20
120626142000 20 a1 00 27 18 00 35 00
120626352000 21 81 00 27 18 20 35 00
120626862000 00 a0 00 a7 18 a0 03 20
120627072000 20 a1 00 27 18 00 35 00
120627282000 21 81 00 27 18 20 35 00
120627912000 00 a0 00 a7 18 a0 03 20
120628122000 20 a1 00 27 18 00 35 00
120628332000 21 81 00 27 18 20 35 00
120628932000 00 a0 00 a7 18 a0 03 20
120629142000 20 a1 00 27 18 00 35 00
120629352000 21 81 00 27 18 20 35 00
120629982000 00 a0 00 a7 18 a0 03 20
120630192000 20 a1 00 27 18 00 35 00
120630402000 21 81 00 27 18 20 35 00
120631002000 00 a0 00 a7 18 a0 03 20
120631242000 20 a1 00 27 18 00 35 00
120631452000 21 81 00 27 18 20 35 00
120632053000 00 a0 00 a7 18 a0 03 20
120632262000 20 a1 00 27 18 00 35 00
120632473000 21 81 00 27 18 20 35 00
120632982000 00 a0 00 a7 18 a0 03 20
120633192000 20 a1 00 27 18 00 35 00
120633432000 21 81 00 27 18 20 35 00
120634032000 00 a0 00 a7 18 a0 03 20
120634242000 20 a1 00 27 18 00 35 00
120634452000 21 81 00 27 18 20 35 00
120634962000 00 a0 00 27 18 a0 03 20
120635172000 20 a1 10 00 00 00 00 00
120635412000 21 81 10 00 00 00 00 00
120636012000 00 a0 10 00 00 00 00 00
120636222000 20 a1 00 27 18 00 35 00
120636432000 21 81 00 27 18 20 35 00
120637063000 00 a0 00 27 18 a0 03 20
120637272000 20 a1 00 27 18 00 35 00
120637512000 21 81 00 27 18 20 35 00
120637992000 00 a0 00 27 18 a0 03 20
120638202000 20 a1 00 27 18 00 35 00
120638412000 21 81 00 27 18 20 35 00
120639042000 00 a0 00 27 18 a0 03 20
120639252000 20 a1 00 27 18 00 35 00
120639462000 21 81 00 27 18 20 35 00
120640062000 00 a0 00 27 18 a0 03 20
120640302000 20 a1 00 27 18 00 35 00
120640512000 21 81 00 27 18 20 35 00
120641112000 00 a0 00 27 18 a0 03 20
120641322000 20 a1 00 27 18 00 35 00
120641532000 21 81 00 27 18 20 35 00
120642162000 00 a0 00 27 18 a0 03 20
120642372000 20 a1 00 27 18 00 35 00
120642582000 21 81 00 27 18 20 35 00
120643212000 00 a0 00 27 18 a0 03 20
120643422000 20 a1 00 27 18 00 35 00
`;
