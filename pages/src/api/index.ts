import { initApi } from "./api_base";
import { app } from "./app";
import { user } from "./user";

export const api = {
  app,
  user,
};
initApi("", api);
