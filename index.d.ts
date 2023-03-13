/// <reference types="node" />

declare class RapidJSON {
  constructor(RAMUsage?: number);

  stringify(payload: any): string;
  parse(payload: string): Object;
  parseBigInt(payload: string): Object;
  forceBigInt(arr: Array<string>): Object;
}

export = RapidJSON;
