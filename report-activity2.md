# Atividade 2 - Dumbbell com TCP Reno e Cubic

## Objetivo

Implementar em ns-3 uma topologia dumbbell com dois fluxos concorrentes:

- um fluxo TCP com taxa oferecida de 5 Mbps;
- um fluxo UDP de fundo com taxa oferecida de 8 Mbps.

O enlace entre os roteadores foi configurado com 10 Mbps e 20 ms de atraso, e os enlaces de acesso com 10 Mbps e 40 ms de atraso.

## Implementação

O cenário foi implementado em [activity2-dumbbell.cc](activity2-dumbbell.cc).

O programa usa:

- `PointToPointDumbbellHelper` para montar a topologia;
- `OnOffHelper` para gerar tráfego TCP e UDP constantes;
- `PacketSinkHelper` para receber os fluxos;
- trace de `CongestionWindow` para registrar a evolução da janela TCP;
- amostragem periódica do throughput do fluxo TCP em arquivo texto.

O fluxo TCP foi executado duas vezes, uma com `TcpNewReno` e outra com `TcpCubic`.

## Saídas geradas

Cada execução gera dois arquivos em `results/`:

- `*-throughput.dat` com o throughput do fluxo TCP ao longo do tempo;
- `*-cwnd.dat` com a evolução da janela de congestionamento.

No repositório, os arquivos finais ficam em [results](results).

## Resultado observado

Foram executadas duas simulações, uma com `TcpNewReno` e outra com `TcpCubic`. Os principais valores observados foram:

| Variante | Bytes recebidos no TCP | Throughput médio | Pico de CWND | CWND final |
| --- | ---: | ---: | ---: | ---: |
| Reno | 6.408.000 | 2,70 Mbps | 346.072 bytes | 143.755 bytes |
| Cubic | 7.764.000 | 3,27 Mbps | 130.320 bytes | 128.872 bytes |

O comportamento observado foi o seguinte:

- o fluxo UDP ocupa uma fração fixa da capacidade do enlace compartilhado;
- o fluxo TCP disputa o restante da banda;
- `TCP Reno` apresentou maior variação da janela de congestionamento, mas convergiu para um throughput médio menor;
- `TCP Cubic` entregou maior throughput médio no mesmo cenário, aproximadamente 21% acima do Reno.

Como o UDP mantém 8 Mbps em um gargalo de 10 Mbps, sobra cerca de 2 Mbps para o fluxo TCP. Assim, o throughput do TCP converge para valores próximos disso, com pequenas diferenças de estabilidade entre as variantes.

## Conclusão

A atividade mostra a diferença prática entre os algoritmos de controle de congestionamento:

- `Reno` foi mais sensível às contenções do enlace e terminou com throughput médio inferior;
- `Cubic` aproveitou melhor a banda residual e manteve maior boa-vazão média.

Em um cenário com tráfego UDP concorrente, `Cubic` produziu melhor desempenho médio para o fluxo TCP na topologia dumbbell configurada.