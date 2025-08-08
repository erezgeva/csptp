/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief Inplement library calls for system depends unit tests
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern void initLibSys(void);
extern void useTestMode(bool);
extern void setMono(long t);
extern void setReal(long t);
extern const char *getOut();
extern const char *getErr();
extern const char *getLog();
