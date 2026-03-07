import { initApi } from "./api_base";
import { user } from "./user";

export const api = {
  user,
};
initApi("", api);
