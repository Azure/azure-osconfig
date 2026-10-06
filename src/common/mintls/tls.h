// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 * OSConfig MinTls core, derived from Mbed TLS 3.6.7.
 * Modified by Microsoft for OSConfig on 2026-10-06:
 * consolidated retained sources; preserved the fixed profile and diagnostics.
 * Original-file provenance and upstream notices remain at each section.
 * OSConfig uses the Apache-2.0 option. Original license texts follow.
 *
 * Mbed TLS files are provided under a dual [Apache-2.0](https://spdx.org/licenses/Apache-2.0.html)
 * OR [GPL-2.0-or-later](https://spdx.org/licenses/GPL-2.0-or-later.html) license.
 * This means that users may choose which of these licenses they take the code
 * under.
 *
 * The full text of each of these licenses is given below.
 *
 *
 *                                  Apache License
 *                            Version 2.0, January 2004
 *                         http://www.apache.org/licenses/
 *
 *    TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION
 *
 *    1. Definitions.
 *
 *       "License" shall mean the terms and conditions for use, reproduction,
 *       and distribution as defined by Sections 1 through 9 of this document.
 *
 *       "Licensor" shall mean the copyright owner or entity authorized by
 *       the copyright owner that is granting the License.
 *
 *       "Legal Entity" shall mean the union of the acting entity and all
 *       other entities that control, are controlled by, or are under common
 *       control with that entity. For the purposes of this definition,
 *       "control" means (i) the power, direct or indirect, to cause the
 *       direction or management of such entity, whether by contract or
 *       otherwise, or (ii) ownership of fifty percent (50%) or more of the
 *       outstanding shares, or (iii) beneficial ownership of such entity.
 *
 *       "You" (or "Your") shall mean an individual or Legal Entity
 *       exercising permissions granted by this License.
 *
 *       "Source" form shall mean the preferred form for making modifications,
 *       including but not limited to software source code, documentation
 *       source, and configuration files.
 *
 *       "Object" form shall mean any form resulting from mechanical
 *       transformation or translation of a Source form, including but
 *       not limited to compiled object code, generated documentation,
 *       and conversions to other media types.
 *
 *       "Work" shall mean the work of authorship, whether in Source or
 *       Object form, made available under the License, as indicated by a
 *       copyright notice that is included in or attached to the work
 *       (an example is provided in the Appendix below).
 *
 *       "Derivative Works" shall mean any work, whether in Source or Object
 *       form, that is based on (or derived from) the Work and for which the
 *       editorial revisions, annotations, elaborations, or other modifications
 *       represent, as a whole, an original work of authorship. For the purposes
 *       of this License, Derivative Works shall not include works that remain
 *       separable from, or merely link (or bind by name) to the interfaces of,
 *       the Work and Derivative Works thereof.
 *
 *       "Contribution" shall mean any work of authorship, including
 *       the original version of the Work and any modifications or additions
 *       to that Work or Derivative Works thereof, that is intentionally
 *       submitted to Licensor for inclusion in the Work by the copyright owner
 *       or by an individual or Legal Entity authorized to submit on behalf of
 *       the copyright owner. For the purposes of this definition, "submitted"
 *       means any form of electronic, verbal, or written communication sent
 *       to the Licensor or its representatives, including but not limited to
 *       communication on electronic mailing lists, source code control systems,
 *       and issue tracking systems that are managed by, or on behalf of, the
 *       Licensor for the purpose of discussing and improving the Work, but
 *       excluding communication that is conspicuously marked or otherwise
 *       designated in writing by the copyright owner as "Not a Contribution."
 *
 *       "Contributor" shall mean Licensor and any individual or Legal Entity
 *       on behalf of whom a Contribution has been received by Licensor and
 *       subsequently incorporated within the Work.
 *
 *    2. Grant of Copyright License. Subject to the terms and conditions of
 *       this License, each Contributor hereby grants to You a perpetual,
 *       worldwide, non-exclusive, no-charge, royalty-free, irrevocable
 *       copyright license to reproduce, prepare Derivative Works of,
 *       publicly display, publicly perform, sublicense, and distribute the
 *       Work and such Derivative Works in Source or Object form.
 *
 *    3. Grant of Patent License. Subject to the terms and conditions of
 *       this License, each Contributor hereby grants to You a perpetual,
 *       worldwide, non-exclusive, no-charge, royalty-free, irrevocable
 *       (except as stated in this section) patent license to make, have made,
 *       use, offer to sell, sell, import, and otherwise transfer the Work,
 *       where such license applies only to those patent claims licensable
 *       by such Contributor that are necessarily infringed by their
 *       Contribution(s) alone or by combination of their Contribution(s)
 *       with the Work to which such Contribution(s) was submitted. If You
 *       institute patent litigation against any entity (including a
 *       cross-claim or counterclaim in a lawsuit) alleging that the Work
 *       or a Contribution incorporated within the Work constitutes direct
 *       or contributory patent infringement, then any patent licenses
 *       granted to You under this License for that Work shall terminate
 *       as of the date such litigation is filed.
 *
 *    4. Redistribution. You may reproduce and distribute copies of the
 *       Work or Derivative Works thereof in any medium, with or without
 *       modifications, and in Source or Object form, provided that You
 *       meet the following conditions:
 *
 *       (a) You must give any other recipients of the Work or
 *           Derivative Works a copy of this License; and
 *
 *       (b) You must cause any modified files to carry prominent notices
 *           stating that You changed the files; and
 *
 *       (c) You must retain, in the Source form of any Derivative Works
 *           that You distribute, all copyright, patent, trademark, and
 *           attribution notices from the Source form of the Work,
 *           excluding those notices that do not pertain to any part of
 *           the Derivative Works; and
 *
 *       (d) If the Work includes a "NOTICE" text file as part of its
 *           distribution, then any Derivative Works that You distribute must
 *           include a readable copy of the attribution notices contained
 *           within such NOTICE file, excluding those notices that do not
 *           pertain to any part of the Derivative Works, in at least one
 *           of the following places: within a NOTICE text file distributed
 *           as part of the Derivative Works; within the Source form or
 *           documentation, if provided along with the Derivative Works; or,
 *           within a display generated by the Derivative Works, if and
 *           wherever such third-party notices normally appear. The contents
 *           of the NOTICE file are for informational purposes only and
 *           do not modify the License. You may add Your own attribution
 *           notices within Derivative Works that You distribute, alongside
 *           or as an addendum to the NOTICE text from the Work, provided
 *           that such additional attribution notices cannot be construed
 *           as modifying the License.
 *
 *       You may add Your own copyright statement to Your modifications and
 *       may provide additional or different license terms and conditions
 *       for use, reproduction, or distribution of Your modifications, or
 *       for any such Derivative Works as a whole, provided Your use,
 *       reproduction, and distribution of the Work otherwise complies with
 *       the conditions stated in this License.
 *
 *    5. Submission of Contributions. Unless You explicitly state otherwise,
 *       any Contribution intentionally submitted for inclusion in the Work
 *       by You to the Licensor shall be under the terms and conditions of
 *       this License, without any additional terms or conditions.
 *       Notwithstanding the above, nothing herein shall supersede or modify
 *       the terms of any separate license agreement you may have executed
 *       with Licensor regarding such Contributions.
 *
 *    6. Trademarks. This License does not grant permission to use the trade
 *       names, trademarks, service marks, or product names of the Licensor,
 *       except as required for reasonable and customary use in describing the
 *       origin of the Work and reproducing the content of the NOTICE file.
 *
 *    7. Disclaimer of Warranty. Unless required by applicable law or
 *       agreed to in writing, Licensor provides the Work (and each
 *       Contributor provides its Contributions) on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *       implied, including, without limitation, any warranties or conditions
 *       of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
 *       PARTICULAR PURPOSE. You are solely responsible for determining the
 *       appropriateness of using or redistributing the Work and assume any
 *       risks associated with Your exercise of permissions under this License.
 *
 *    8. Limitation of Liability. In no event and under no legal theory,
 *       whether in tort (including negligence), contract, or otherwise,
 *       unless required by applicable law (such as deliberate and grossly
 *       negligent acts) or agreed to in writing, shall any Contributor be
 *       liable to You for damages, including any direct, indirect, special,
 *       incidental, or consequential damages of any character arising as a
 *       result of this License or out of the use or inability to use the
 *       Work (including but not limited to damages for loss of goodwill,
 *       work stoppage, computer failure or malfunction, or any and all
 *       other commercial damages or losses), even if such Contributor
 *       has been advised of the possibility of such damages.
 *
 *    9. Accepting Warranty or Additional Liability. While redistributing
 *       the Work or Derivative Works thereof, You may choose to offer,
 *       and charge a fee for, acceptance of support, warranty, indemnity,
 *       or other liability obligations and/or rights consistent with this
 *       License. However, in accepting such obligations, You may act only
 *       on Your own behalf and on Your sole responsibility, not on behalf
 *       of any other Contributor, and only if You agree to indemnify,
 *       defend, and hold each Contributor harmless for any liability
 *       incurred by, or claims asserted against, such Contributor by reason
 *       of your accepting any such warranty or additional liability.
 *
 *    END OF TERMS AND CONDITIONS
 *
 *    APPENDIX: How to apply the Apache License to your work.
 *
 *       To apply the Apache License to your work, attach the following
 *       boilerplate notice, with the fields enclosed by brackets "[]"
 *       replaced with your own identifying information. (Don't include
 *       the brackets!)  The text should be enclosed in the appropriate
 *       comment syntax for the file format. We also recommend that a
 *       file or class name and description of purpose be included on the
 *       same "printed page" as the copyright notice for easier
 *       identification within third-party archives.
 *
 *    Copyright [yyyy] [name of copyright owner]
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *
 * ===============================================================================
 *
 *
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *                             Preamble
 *
 *   The licenses for most software are designed to take away your
 * freedom to share and change it.  By contrast, the GNU General Public
 * License is intended to guarantee your freedom to share and change free
 * software--to make sure the software is free for all its users.  This
 * General Public License applies to most of the Free Software
 * Foundation's software and to any other program whose authors commit to
 * using it.  (Some other Free Software Foundation software is covered by
 * the GNU Lesser General Public License instead.)  You can apply it to
 * your programs, too.
 *
 *   When we speak of free software, we are referring to freedom, not
 * price.  Our General Public Licenses are designed to make sure that you
 * have the freedom to distribute copies of free software (and charge for
 * this service if you wish), that you receive source code or can get it
 * if you want it, that you can change the software or use pieces of it
 * in new free programs; and that you know you can do these things.
 *
 *   To protect your rights, we need to make restrictions that forbid
 * anyone to deny you these rights or to ask you to surrender the rights.
 * These restrictions translate to certain responsibilities for you if you
 * distribute copies of the software, or if you modify it.
 *
 *   For example, if you distribute copies of such a program, whether
 * gratis or for a fee, you must give the recipients all the rights that
 * you have.  You must make sure that they, too, receive or can get the
 * source code.  And you must show them these terms so they know their
 * rights.
 *
 *   We protect your rights with two steps: (1) copyright the software, and
 * (2) offer you this license which gives you legal permission to copy,
 * distribute and/or modify the software.
 *
 *   Also, for each author's protection and ours, we want to make certain
 * that everyone understands that there is no warranty for this free
 * software.  If the software is modified by someone else and passed on, we
 * want its recipients to know that what they have is not the original, so
 * that any problems introduced by others will not reflect on the original
 * authors' reputations.
 *
 *   Finally, any free program is threatened constantly by software
 * patents.  We wish to avoid the danger that redistributors of a free
 * program will individually obtain patent licenses, in effect making the
 * program proprietary.  To prevent this, we have made it clear that any
 * patent must be licensed for everyone's free use or not licensed at all.
 *
 *   The precise terms and conditions for copying, distribution and
 * modification follow.
 *
 *                     GNU GENERAL PUBLIC LICENSE
 *    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *   0. This License applies to any program or other work which contains
 * a notice placed by the copyright holder saying it may be distributed
 * under the terms of this General Public License.  The "Program", below,
 * refers to any such program or work, and a "work based on the Program"
 * means either the Program or any derivative work under copyright law:
 * that is to say, a work containing the Program or a portion of it,
 * either verbatim or with modifications and/or translated into another
 * language.  (Hereinafter, translation is included without limitation in
 * the term "modification".)  Each licensee is addressed as "you".
 *
 * Activities other than copying, distribution and modification are not
 * covered by this License; they are outside its scope.  The act of
 * running the Program is not restricted, and the output from the Program
 * is covered only if its contents constitute a work based on the
 * Program (independent of having been made by running the Program).
 * Whether that is true depends on what the Program does.
 *
 *   1. You may copy and distribute verbatim copies of the Program's
 * source code as you receive it, in any medium, provided that you
 * conspicuously and appropriately publish on each copy an appropriate
 * copyright notice and disclaimer of warranty; keep intact all the
 * notices that refer to this License and to the absence of any warranty;
 * and give any other recipients of the Program a copy of this License
 * along with the Program.
 *
 * You may charge a fee for the physical act of transferring a copy, and
 * you may at your option offer warranty protection in exchange for a fee.
 *
 *   2. You may modify your copy or copies of the Program or any portion
 * of it, thus forming a work based on the Program, and copy and
 * distribute such modifications or work under the terms of Section 1
 * above, provided that you also meet all of these conditions:
 *
 *     a) You must cause the modified files to carry prominent notices
 *     stating that you changed the files and the date of any change.
 *
 *     b) You must cause any work that you distribute or publish, that in
 *     whole or in part contains or is derived from the Program or any
 *     part thereof, to be licensed as a whole at no charge to all third
 *     parties under the terms of this License.
 *
 *     c) If the modified program normally reads commands interactively
 *     when run, you must cause it, when started running for such
 *     interactive use in the most ordinary way, to print or display an
 *     announcement including an appropriate copyright notice and a
 *     notice that there is no warranty (or else, saying that you provide
 *     a warranty) and that users may redistribute the program under
 *     these conditions, and telling the user how to view a copy of this
 *     License.  (Exception: if the Program itself is interactive but
 *     does not normally print such an announcement, your work based on
 *     the Program is not required to print an announcement.)
 *
 * These requirements apply to the modified work as a whole.  If
 * identifiable sections of that work are not derived from the Program,
 * and can be reasonably considered independent and separate works in
 * themselves, then this License, and its terms, do not apply to those
 * sections when you distribute them as separate works.  But when you
 * distribute the same sections as part of a whole which is a work based
 * on the Program, the distribution of the whole must be on the terms of
 * this License, whose permissions for other licensees extend to the
 * entire whole, and thus to each and every part regardless of who wrote it.
 *
 * Thus, it is not the intent of this section to claim rights or contest
 * your rights to work written entirely by you; rather, the intent is to
 * exercise the right to control the distribution of derivative or
 * collective works based on the Program.
 *
 * In addition, mere aggregation of another work not based on the Program
 * with the Program (or with a work based on the Program) on a volume of
 * a storage or distribution medium does not bring the other work under
 * the scope of this License.
 *
 *   3. You may copy and distribute the Program (or a work based on it,
 * under Section 2) in object code or executable form under the terms of
 * Sections 1 and 2 above provided that you also do one of the following:
 *
 *     a) Accompany it with the complete corresponding machine-readable
 *     source code, which must be distributed under the terms of Sections
 *     1 and 2 above on a medium customarily used for software interchange; or,
 *
 *     b) Accompany it with a written offer, valid for at least three
 *     years, to give any third party, for a charge no more than your
 *     cost of physically performing source distribution, a complete
 *     machine-readable copy of the corresponding source code, to be
 *     distributed under the terms of Sections 1 and 2 above on a medium
 *     customarily used for software interchange; or,
 *
 *     c) Accompany it with the information you received as to the offer
 *     to distribute corresponding source code.  (This alternative is
 *     allowed only for noncommercial distribution and only if you
 *     received the program in object code or executable form with such
 *     an offer, in accord with Subsection b above.)
 *
 * The source code for a work means the preferred form of the work for
 * making modifications to it.  For an executable work, complete source
 * code means all the source code for all modules it contains, plus any
 * associated interface definition files, plus the scripts used to
 * control compilation and installation of the executable.  However, as a
 * special exception, the source code distributed need not include
 * anything that is normally distributed (in either source or binary
 * form) with the major components (compiler, kernel, and so on) of the
 * operating system on which the executable runs, unless that component
 * itself accompanies the executable.
 *
 * If distribution of executable or object code is made by offering
 * access to copy from a designated place, then offering equivalent
 * access to copy the source code from the same place counts as
 * distribution of the source code, even though third parties are not
 * compelled to copy the source along with the object code.
 *
 *   4. You may not copy, modify, sublicense, or distribute the Program
 * except as expressly provided under this License.  Any attempt
 * otherwise to copy, modify, sublicense or distribute the Program is
 * void, and will automatically terminate your rights under this License.
 * However, parties who have received copies, or rights, from you under
 * this License will not have their licenses terminated so long as such
 * parties remain in full compliance.
 *
 *   5. You are not required to accept this License, since you have not
 * signed it.  However, nothing else grants you permission to modify or
 * distribute the Program or its derivative works.  These actions are
 * prohibited by law if you do not accept this License.  Therefore, by
 * modifying or distributing the Program (or any work based on the
 * Program), you indicate your acceptance of this License to do so, and
 * all its terms and conditions for copying, distributing or modifying
 * the Program or works based on it.
 *
 *   6. Each time you redistribute the Program (or any work based on the
 * Program), the recipient automatically receives a license from the
 * original licensor to copy, distribute or modify the Program subject to
 * these terms and conditions.  You may not impose any further
 * restrictions on the recipients' exercise of the rights granted herein.
 * You are not responsible for enforcing compliance by third parties to
 * this License.
 *
 *   7. If, as a consequence of a court judgment or allegation of patent
 * infringement or for any other reason (not limited to patent issues),
 * conditions are imposed on you (whether by court order, agreement or
 * otherwise) that contradict the conditions of this License, they do not
 * excuse you from the conditions of this License.  If you cannot
 * distribute so as to satisfy simultaneously your obligations under this
 * License and any other pertinent obligations, then as a consequence you
 * may not distribute the Program at all.  For example, if a patent
 * license would not permit royalty-free redistribution of the Program by
 * all those who receive copies directly or indirectly through you, then
 * the only way you could satisfy both it and this License would be to
 * refrain entirely from distribution of the Program.
 *
 * If any portion of this section is held invalid or unenforceable under
 * any particular circumstance, the balance of the section is intended to
 * apply and the section as a whole is intended to apply in other
 * circumstances.
 *
 * It is not the purpose of this section to induce you to infringe any
 * patents or other property right claims or to contest validity of any
 * such claims; this section has the sole purpose of protecting the
 * integrity of the free software distribution system, which is
 * implemented by public license practices.  Many people have made
 * generous contributions to the wide range of software distributed
 * through that system in reliance on consistent application of that
 * system; it is up to the author/donor to decide if he or she is willing
 * to distribute software through any other system and a licensee cannot
 * impose that choice.
 *
 * This section is intended to make thoroughly clear what is believed to
 * be a consequence of the rest of this License.
 *
 *   8. If the distribution and/or use of the Program is restricted in
 * certain countries either by patents or by copyrighted interfaces, the
 * original copyright holder who places the Program under this License
 * may add an explicit geographical distribution limitation excluding
 * those countries, so that distribution is permitted only in or among
 * countries not thus excluded.  In such case, this License incorporates
 * the limitation as if written in the body of this License.
 *
 *   9. The Free Software Foundation may publish revised and/or new versions
 * of the General Public License from time to time.  Such new versions will
 * be similar in spirit to the present version, but may differ in detail to
 * address new problems or concerns.
 *
 * Each version is given a distinguishing version number.  If the Program
 * specifies a version number of this License which applies to it and "any
 * later version", you have the option of following the terms and conditions
 * either of that version or of any later version published by the Free
 * Software Foundation.  If the Program does not specify a version number of
 * this License, you may choose any version ever published by the Free Software
 * Foundation.
 *
 *   10. If you wish to incorporate parts of the Program into other free
 * programs whose distribution conditions are different, write to the author
 * to ask for permission.  For software which is copyrighted by the Free
 * Software Foundation, write to the Free Software Foundation; we sometimes
 * make exceptions for this.  Our decision will be guided by the two goals
 * of preserving the free status of all derivatives of our free software and
 * of promoting the sharing and reuse of software generally.
 *
 *                             NO WARRANTY
 *
 *   11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
 * FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
 * OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
 * PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
 * OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
 * TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
 * PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
 * REPAIR OR CORRECTION.
 *
 *   12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
 * WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
 * REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
 * INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
 * OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
 * TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
 * YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
 * PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 *                      END OF TERMS AND CONDITIONS
 *
 *             How to Apply These Terms to Your New Programs
 *
 *   If you develop a new program, and you want it to be of the greatest
 * possible use to the public, the best way to achieve this is to make it
 * free software which everyone can redistribute and change under these terms.
 *
 *   To do so, attach the following notices to the program.  It is safest
 * to attach them to the start of each source file to most effectively
 * convey the exclusion of warranty; and each file should have at least
 * the "copyright" line and a pointer to where the full notice is found.
 *
 *     <one line to give the program's name and a brief idea of what it does.>
 *     Copyright (C) <year>  <name of author>
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License along
 *     with this program; if not, write to the Free Software Foundation, Inc.,
 *     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Also add information on how to contact you by electronic and paper mail.
 *
 * If the program is interactive, make it output a short notice like this
 * when it starts in an interactive mode:
 *
 *     Gnomovision version 69, Copyright (C) year name of author
 *     Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 *     This is free software, and you are welcome to redistribute it
 *     under certain conditions; type `show c' for details.
 *
 * The hypothetical commands `show w' and `show c' should show the appropriate
 * parts of the General Public License.  Of course, the commands you use may
 * be called something other than `show w' and `show c'; they could even be
 * mouse-clicks or menu items--whatever suits your program.
 *
 * You should also get your employer (if you work as a programmer) or your
 * school, if any, to sign a "copyright disclaimer" for the program, if
 * necessary.  Here is a sample; alter the names:
 *
 *   Yoyodyne, Inc., hereby disclaims all copyright interest in the program
 *   `Gnomovision' (which makes passes at compilers) written by James Hacker.
 *
 *   <signature of Ty Coon>, 1 April 1989
 *   Ty Coon, President of Vice
 *
 * This General Public License does not permit incorporating your program into
 * proprietary programs.  If your program is a subroutine library, you may
 * consider it more useful to permit linking proprietary applications with the
 * library.  If this is what you want to do, use the GNU Lesser General
 * Public License instead of this License.
 */

#ifndef OSCONFIG_MIN_TLS_CORE_H
#define OSCONFIG_MIN_TLS_CORE_H

/* ===== Retained common.h ===== */

/**
 * \file common.h
 *
 * \brief Utility macros for internal use in the library
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/common.h.
 */

#ifndef MBEDTLS_LIBRARY_COMMON_H
#define MBEDTLS_LIBRARY_COMMON_H

/* ===== Retained build_info.h ===== */

/**
 * \file mbedtls/build_info.h
 *
 * \brief Build-time configuration info
 *
 *  Include this file if you need to depend on the
 *  configuration options defined in mbedtls_config.h or MBEDTLS_CONFIG_FILE
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/build_info.h.
 */

#ifndef MBEDTLS_BUILD_INFO_H
#define MBEDTLS_BUILD_INFO_H

#include "MinTlsLog.h"

/*
 * This set of compile-time defines can be used to determine the version number
 * of the Mbed TLS library used. Run-time variables for the same can be found in
 * version.h
 */

/**
 * The version number x.y.z is split into three parts.
 * Major, Minor, Patchlevel
 */
#define MBEDTLS_VERSION_MAJOR  3
#define MBEDTLS_VERSION_MINOR  6
#define MBEDTLS_VERSION_PATCH  7

/**
 * The single version number has the following structure:
 *    MMNNPP00
 *    Major version | Minor version | Patch version
 */
#define MBEDTLS_VERSION_NUMBER         0x03060700
#define MBEDTLS_VERSION_STRING         "3.6.7"
#define MBEDTLS_VERSION_STRING_FULL    "Mbed TLS 3.6.7"

/* Macros for build-time platform detection */

#if !defined(MBEDTLS_ARCH_IS_ARM64) && \
    (defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC))
#define MBEDTLS_ARCH_IS_ARM64
#endif

#if !defined(MBEDTLS_ARCH_IS_ARM32) && \
    (defined(__arm__) || defined(_M_ARM) || \
    defined(_M_ARMT) || defined(__thumb__) || defined(__thumb2__))
#define MBEDTLS_ARCH_IS_ARM32
#endif

#if !defined(MBEDTLS_ARCH_IS_X64) && \
    (defined(__amd64__) || defined(__x86_64__) || \
    ((defined(_M_X64) || defined(_M_AMD64)) && !defined(_M_ARM64EC)))
#define MBEDTLS_ARCH_IS_X64
#endif

#if !defined(MBEDTLS_ARCH_IS_X86) && \
    (defined(__i386__) || defined(_X86_) || \
    (defined(_M_IX86) && !defined(_M_I86)))
#define MBEDTLS_ARCH_IS_X86
#endif

#if !defined(MBEDTLS_PLATFORM_IS_WINDOWS_ON_ARM64) && \
    (defined(_M_ARM64) || defined(_M_ARM64EC))
#define MBEDTLS_PLATFORM_IS_WINDOWS_ON_ARM64
#endif

/* This is defined if the architecture is Armv8-A, or higher */
#if !defined(MBEDTLS_ARCH_IS_ARMV8_A)
#if defined(__ARM_ARCH) && defined(__ARM_ARCH_PROFILE)
#if (__ARM_ARCH >= 8) && (__ARM_ARCH_PROFILE == 'A')
/* GCC, clang, armclang and IAR */
#define MBEDTLS_ARCH_IS_ARMV8_A
#endif
#elif defined(__ARM_ARCH_8A)
/* Alternative defined by clang */
#define MBEDTLS_ARCH_IS_ARMV8_A
#elif defined(_M_ARM64) || defined(_M_ARM64EC)
/* MSVC ARM64 is at least Armv8.0-A */
#define MBEDTLS_ARCH_IS_ARMV8_A
#endif
#endif

#if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__clang__) \
    && !defined(__llvm__) && !defined(__INTEL_COMPILER)
/* Defined if the compiler really is gcc and not clang, etc */
#define MBEDTLS_COMPILER_IS_GCC
#define MBEDTLS_GCC_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/* Define `inline` on some non-C99-compliant compilers. */
#if (defined(__ARMCC_VERSION) || defined(_MSC_VER)) && \
    !defined(inline) && !defined(__cplusplus)
#define inline __inline
#endif

#if defined(MBEDTLS_CONFIG_FILES_READ)
#error "Something went wrong: MBEDTLS_CONFIG_FILES_READ defined before reading the config files!"
#endif
#if defined(MBEDTLS_CONFIG_IS_FINALIZED)
#error "Something went wrong: MBEDTLS_CONFIG_IS_FINALIZED defined before reading the config files!"
#endif

/* OSConfig maintains one private profile, not a configurable TLS distribution. */
#include "MinTlsConfig.h"

#if defined(MBEDTLS_CONFIG_VERSION) && ( \
    MBEDTLS_CONFIG_VERSION < 0x03000000 || \
                             MBEDTLS_CONFIG_VERSION > MBEDTLS_VERSION_NUMBER)
#error "Invalid config version, defined value of MBEDTLS_CONFIG_VERSION is unsupported"
#endif

/* Indicate that all configuration files have been read.
 * It is now time to adjust the configuration (follow through on dependencies,
 * make PSA and legacy crypto consistent, etc.).
 */
#define MBEDTLS_CONFIG_FILES_READ

/* Auto-enable MBEDTLS_CTR_DRBG_USE_128_BIT_KEY if
 * MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH and MBEDTLS_CTR_DRBG_C defined
 * to ensure a 128-bit key size in CTR_DRBG.
 */
#if defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH) && defined(MBEDTLS_CTR_DRBG_C)
#define MBEDTLS_CTR_DRBG_USE_128_BIT_KEY
#endif

/* Auto-enable MBEDTLS_MD_C if needed by a module that didn't require it
 * in a previous release, to ensure backwards compatibility.
 */
#if defined(MBEDTLS_PKCS5_C)
#define MBEDTLS_MD_C
#endif

/* ===== Retained config_adjust_legacy_crypto.h ===== */

/**
 * \file mbedtls/config_adjust_legacy_crypto.h
 * \brief Adjust legacy configuration configuration
 *
 * This is an internal header. Do not include it directly.
 *
 * Automatically enable certain dependencies. Generally, MBEDTLS_xxx
 * configurations need to be explicitly enabled by the user: enabling
 * MBEDTLS_xxx_A but not MBEDTLS_xxx_B when A requires B results in a
 * compilation error. However, we do automatically enable certain options
 * in some circumstances. One case is if MBEDTLS_xxx_B is an internal option
 * used to identify parts of a module that are used by other module, and we
 * don't want to make the symbol MBEDTLS_xxx_B part of the public API.
 * Another case is if A didn't depend on B in earlier versions, and we
 * want to use B in A but we need to preserve backward compatibility with
 * configurations that explicitly activate MBEDTLS_xxx_A but not
 * MBEDTLS_xxx_B.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/config_adjust_legacy_crypto.h.
 */

#ifndef MBEDTLS_CONFIG_ADJUST_LEGACY_CRYPTO_H
#define MBEDTLS_CONFIG_ADJUST_LEGACY_CRYPTO_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include mbedtls/config_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif /* */

/* Ideally, we'd set those as defaults in mbedtls_config.h, but
 * putting an #ifdef _WIN32 in mbedtls_config.h would confuse config.py.
 *
 * So, adjust it here.
 * Not related to crypto, but this is the bottom of the stack. */
#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER <= 1900)
#if !defined(MBEDTLS_PLATFORM_SNPRINTF_ALT) && \
    !defined(MBEDTLS_PLATFORM_SNPRINTF_MACRO)
#define MBEDTLS_PLATFORM_SNPRINTF_ALT
#endif
#if !defined(MBEDTLS_PLATFORM_VSNPRINTF_ALT) && \
    !defined(MBEDTLS_PLATFORM_VSNPRINTF_MACRO)
#define MBEDTLS_PLATFORM_VSNPRINTF_ALT
#endif
#endif /* _MINGW32__ || (_MSC_VER && (_MSC_VER <= 1900)) */

/* The number of "true" entropy sources (excluding NV seed).
 * This must be consistent with mbedtls_entropy_init() in entropy.c.
 */
/* Define auxiliary macros, because in standard C, defined(xxx) is only
 * allowed directly on an #if or #elif line, not in recursive expansion. */
#if defined(MBEDTLS_NO_PLATFORM_ENTROPY)
#define MBEDTLS_PLATFORM_ENTROPY_ENABLED 0
#else
#define MBEDTLS_PLATFORM_ENTROPY_ENABLED 1
#endif
#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
#define MBEDTLS_ENTROPY_HARDWARE_ALT_DEFINED 1
#else
#define MBEDTLS_ENTROPY_HARDWARE_ALT_DEFINED 0
#endif

#define MBEDTLS_ENTROPY_TRUE_SOURCES ( \
        MBEDTLS_ENTROPY_HARDWARE_ALT_DEFINED + \
        MBEDTLS_PLATFORM_ENTROPY_ENABLED + \
        0)

/* Whether there is at least one entropy source for the entropy module.
 *
 * Note that when MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG is enabled, the entropy
 * module is unused and the configuration will typically not include any
 * entropy source, so this macro will typically remain undefined.
 */
#if defined(MBEDTLS_ENTROPY_NV_SEED)
#define MBEDTLS_ENTROPY_HAVE_SOURCES (MBEDTLS_ENTROPY_TRUE_SOURCES + 1)
#elif MBEDTLS_ENTROPY_TRUE_SOURCES != 0
#define MBEDTLS_ENTROPY_HAVE_SOURCES MBEDTLS_ENTROPY_TRUE_SOURCES
#else
#undef MBEDTLS_ENTROPY_HAVE_SOURCES
#endif

/* Test function dependencies can only check with defined(),
 * not other preprocessor expressions. */
#if MBEDTLS_ENTROPY_TRUE_SOURCES > 0
#define MBEDTLS_ENTROPY_HAVE_TRUE_SOURCES
#else
#undef MBEDTLS_ENTROPY_HAVE_TRUE_SOURCES
#endif

/* If MBEDTLS_PSA_CRYPTO_C is defined, make sure MBEDTLS_PSA_CRYPTO_CLIENT
 * is defined as well to include all PSA code.
 */
#if defined(MBEDTLS_PSA_CRYPTO_C)
#define MBEDTLS_PSA_CRYPTO_CLIENT
#endif /* MBEDTLS_PSA_CRYPTO_C */

/* Auto-enable CIPHER_C when any of the unauthenticated ciphers is builtin
 * in PSA. */
#if defined(MBEDTLS_PSA_CRYPTO_C) && \
    (defined(MBEDTLS_PSA_BUILTIN_ALG_STREAM_CIPHER) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CTR) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CFB) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_OFB) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_ECB_NO_PADDING) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CBC_NO_PADDING) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CBC_PKCS7) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CCM_STAR_NO_TAG) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_CMAC))
#define MBEDTLS_CIPHER_C
#endif

/* Auto-enable MBEDTLS_MD_LIGHT based on MBEDTLS_MD_C.
 * This allows checking for MD_LIGHT rather than MD_LIGHT || MD_C.
 */
#if defined(MBEDTLS_MD_C)
#define MBEDTLS_MD_LIGHT
#endif

/* Auto-enable MBEDTLS_MD_LIGHT if needed by a module that didn't require it
 * in a previous release, to ensure backwards compatibility.
 */
#if defined(MBEDTLS_ECJPAKE_C) || \
    defined(MBEDTLS_PEM_PARSE_C) || \
    defined(MBEDTLS_ENTROPY_C) || \
    defined(MBEDTLS_PK_C) || \
    defined(MBEDTLS_PKCS12_C) || \
    defined(MBEDTLS_RSA_C) || \
    defined(MBEDTLS_SSL_TLS_C) || \
    defined(MBEDTLS_X509_USE_C) || \
    defined(MBEDTLS_X509_CREATE_C)
#define MBEDTLS_MD_LIGHT
#endif

#if defined(MBEDTLS_MD_LIGHT)
/*
 * - MBEDTLS_MD_CAN_xxx is defined if the md module can perform xxx.
 * - MBEDTLS_MD_xxx_VIA_PSA is defined if the md module may perform xxx via PSA
 *   (see below).
 * - MBEDTLS_MD_SOME_PSA is defined if at least one algorithm may be performed
 *   via PSA (see below).
 * - MBEDTLS_MD_SOME_LEGACY is defined if at least one algorithm may be performed
 *   via a direct legacy call (see below).
 *
 * The md module performs an algorithm via PSA if there is a PSA hash
 * accelerator and the PSA driver subsytem is initialized at the time the
 * operation is started, and makes a direct legacy call otherwise.
 */

/* PSA accelerated implementations */
#if defined(MBEDTLS_PSA_CRYPTO_C)

#if defined(MBEDTLS_PSA_ACCEL_ALG_MD5)
#define MBEDTLS_MD_CAN_MD5
#define MBEDTLS_MD_MD5_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA_1)
#define MBEDTLS_MD_CAN_SHA1
#define MBEDTLS_MD_SHA1_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA_224)
#define MBEDTLS_MD_CAN_SHA224
#define MBEDTLS_MD_SHA224_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA_256)
#define MBEDTLS_MD_CAN_SHA256
#define MBEDTLS_MD_SHA256_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA_384)
#define MBEDTLS_MD_CAN_SHA384
#define MBEDTLS_MD_SHA384_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA_512)
#define MBEDTLS_MD_CAN_SHA512
#define MBEDTLS_MD_SHA512_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_RIPEMD160)
#define MBEDTLS_MD_CAN_RIPEMD160
#define MBEDTLS_MD_RIPEMD160_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_224)
#define MBEDTLS_MD_CAN_SHA3_224
#define MBEDTLS_MD_SHA3_224_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_256)
#define MBEDTLS_MD_CAN_SHA3_256
#define MBEDTLS_MD_SHA3_256_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_384)
#define MBEDTLS_MD_CAN_SHA3_384
#define MBEDTLS_MD_SHA3_384_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_512)
#define MBEDTLS_MD_CAN_SHA3_512
#define MBEDTLS_MD_SHA3_512_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif

#elif defined(MBEDTLS_PSA_CRYPTO_CLIENT)

#if defined(PSA_WANT_ALG_MD5)
#define MBEDTLS_MD_CAN_MD5
#define MBEDTLS_MD_MD5_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA_1)
#define MBEDTLS_MD_CAN_SHA1
#define MBEDTLS_MD_SHA1_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA_224)
#define MBEDTLS_MD_CAN_SHA224
#define MBEDTLS_MD_SHA224_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA_256)
#define MBEDTLS_MD_CAN_SHA256
#define MBEDTLS_MD_SHA256_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA_384)
#define MBEDTLS_MD_CAN_SHA384
#define MBEDTLS_MD_SHA384_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA_512)
#define MBEDTLS_MD_CAN_SHA512
#define MBEDTLS_MD_SHA512_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_RIPEMD160)
#define MBEDTLS_MD_CAN_RIPEMD160
#define MBEDTLS_MD_RIPEMD160_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA3_224)
#define MBEDTLS_MD_CAN_SHA3_224
#define MBEDTLS_MD_SHA3_224_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA3_256)
#define MBEDTLS_MD_CAN_SHA3_256
#define MBEDTLS_MD_SHA3_256_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA3_384)
#define MBEDTLS_MD_CAN_SHA3_384
#define MBEDTLS_MD_SHA3_384_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif
#if defined(PSA_WANT_ALG_SHA3_512)
#define MBEDTLS_MD_CAN_SHA3_512
#define MBEDTLS_MD_SHA3_512_VIA_PSA
#define MBEDTLS_MD_SOME_PSA
#endif

#endif /* !MBEDTLS_PSA_CRYPTO_CLIENT && !MBEDTLS_PSA_CRYPTO_C */

/* Built-in implementations */
#if defined(MBEDTLS_MD5_C)
#define MBEDTLS_MD_CAN_MD5
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA1_C)
#define MBEDTLS_MD_CAN_SHA1
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA224_C)
#define MBEDTLS_MD_CAN_SHA224
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA256_C)
#define MBEDTLS_MD_CAN_SHA256
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA384_C)
#define MBEDTLS_MD_CAN_SHA384
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA512_C)
#define MBEDTLS_MD_CAN_SHA512
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_SHA3_C)
#define MBEDTLS_MD_CAN_SHA3_224
#define MBEDTLS_MD_CAN_SHA3_256
#define MBEDTLS_MD_CAN_SHA3_384
#define MBEDTLS_MD_CAN_SHA3_512
#define MBEDTLS_MD_SOME_LEGACY
#endif
#if defined(MBEDTLS_RIPEMD160_C)
#define MBEDTLS_MD_CAN_RIPEMD160
#define MBEDTLS_MD_SOME_LEGACY
#endif

#endif /* MBEDTLS_MD_LIGHT */

/* BLOCK_CIPHER module can dispatch to PSA when:
 * - PSA is enabled and drivers have been initialized
 * - desired key type is supported on the PSA side
 * If the above conditions are not met, but the legacy support is enabled, then
 * BLOCK_CIPHER will dynamically fallback to it.
 *
 * In case BLOCK_CIPHER is defined (see below) the following symbols/helpers
 * can be used to define its capabilities:
 * - MBEDTLS_BLOCK_CIPHER_SOME_PSA: there is at least 1 key type between AES,
 *   ARIA and Camellia which is supported through a driver;
 * - MBEDTLS_BLOCK_CIPHER_xxx_VIA_PSA: xxx key type is supported through a
 *   driver;
 * - MBEDTLS_BLOCK_CIPHER_xxx_VIA_LEGACY: xxx key type is supported through
 *   a legacy module (i.e. MBEDTLS_xxx_C)
 */
#if defined(MBEDTLS_PSA_CRYPTO_C)
#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_AES)
#define MBEDTLS_BLOCK_CIPHER_AES_VIA_PSA
#define MBEDTLS_BLOCK_CIPHER_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ARIA)
#define MBEDTLS_BLOCK_CIPHER_ARIA_VIA_PSA
#define MBEDTLS_BLOCK_CIPHER_SOME_PSA
#endif
#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_CAMELLIA)
#define MBEDTLS_BLOCK_CIPHER_CAMELLIA_VIA_PSA
#define MBEDTLS_BLOCK_CIPHER_SOME_PSA
#endif
#endif /* MBEDTLS_PSA_CRYPTO_C */

#if defined(MBEDTLS_AES_C)
#define MBEDTLS_BLOCK_CIPHER_AES_VIA_LEGACY
#endif
#if defined(MBEDTLS_ARIA_C)
#define MBEDTLS_BLOCK_CIPHER_ARIA_VIA_LEGACY
#endif
#if defined(MBEDTLS_CAMELLIA_C)
#define MBEDTLS_BLOCK_CIPHER_CAMELLIA_VIA_LEGACY
#endif

/* Helpers to state that BLOCK_CIPHER module supports AES, ARIA and/or Camellia
 * block ciphers via either PSA or legacy. */
#if defined(MBEDTLS_BLOCK_CIPHER_AES_VIA_PSA) || \
    defined(MBEDTLS_BLOCK_CIPHER_AES_VIA_LEGACY)
#define MBEDTLS_BLOCK_CIPHER_CAN_AES
#endif
#if defined(MBEDTLS_BLOCK_CIPHER_ARIA_VIA_PSA) || \
    defined(MBEDTLS_BLOCK_CIPHER_ARIA_VIA_LEGACY)
#define MBEDTLS_BLOCK_CIPHER_CAN_ARIA
#endif
#if defined(MBEDTLS_BLOCK_CIPHER_CAMELLIA_VIA_PSA) || \
    defined(MBEDTLS_BLOCK_CIPHER_CAMELLIA_VIA_LEGACY)
#define MBEDTLS_BLOCK_CIPHER_CAN_CAMELLIA
#endif

/* GCM_C and CCM_C can either depend on (in order of preference) BLOCK_CIPHER_C
 * or CIPHER_C. The former is auto-enabled when:
 * - CIPHER_C is not defined, which is also the legacy solution;
 * - BLOCK_CIPHER_SOME_PSA because in this case BLOCK_CIPHER can take advantage
 *   of the driver's acceleration.
 */
#if (defined(MBEDTLS_GCM_C) || defined(MBEDTLS_CCM_C)) && \
    (!defined(MBEDTLS_CIPHER_C) || defined(MBEDTLS_BLOCK_CIPHER_SOME_PSA))
#define MBEDTLS_BLOCK_CIPHER_C
#endif

/* Helpers for GCM/CCM capabilities */
#if (defined(MBEDTLS_CIPHER_C) && defined(MBEDTLS_AES_C)) || \
    (defined(MBEDTLS_BLOCK_CIPHER_C) && defined(MBEDTLS_BLOCK_CIPHER_CAN_AES))
#define MBEDTLS_CCM_GCM_CAN_AES
#endif

#if (defined(MBEDTLS_CIPHER_C) && defined(MBEDTLS_ARIA_C)) || \
    (defined(MBEDTLS_BLOCK_CIPHER_C) && defined(MBEDTLS_BLOCK_CIPHER_CAN_ARIA))
#define MBEDTLS_CCM_GCM_CAN_ARIA
#endif

#if (defined(MBEDTLS_CIPHER_C) && defined(MBEDTLS_CAMELLIA_C)) || \
    (defined(MBEDTLS_BLOCK_CIPHER_C) && defined(MBEDTLS_BLOCK_CIPHER_CAN_CAMELLIA))
#define MBEDTLS_CCM_GCM_CAN_CAMELLIA
#endif

/* MBEDTLS_ECP_LIGHT is auto-enabled by the following symbols:
 * - MBEDTLS_ECP_C because now it consists of MBEDTLS_ECP_LIGHT plus functions
 *   for curve arithmetic. As a consequence if MBEDTLS_ECP_C is required for
 *   some reason, then MBEDTLS_ECP_LIGHT should be enabled as well.
 * - MBEDTLS_PK_PARSE_EC_EXTENDED and MBEDTLS_PK_PARSE_EC_COMPRESSED because
 *   these features are not supported in PSA so the only way to have them is
 *   to enable the built-in solution.
 *   Both of them are temporary dependencies:
 *   - PK_PARSE_EC_EXTENDED will be removed after #7779 and #7789
 *   - support for compressed points should also be added to PSA, but in this
 *     case there is no associated issue to track it yet.
 * - PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE because Weierstrass key derivation
 *   still depends on ECP_LIGHT.
 * - PK_C + USE_PSA + PSA_WANT_ALG_ECDSA is a temporary dependency which will
 *   be fixed by #7453.
 */
#if defined(MBEDTLS_ECP_C) || \
    defined(MBEDTLS_PK_PARSE_EC_EXTENDED) || \
    defined(MBEDTLS_PK_PARSE_EC_COMPRESSED) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define MBEDTLS_ECP_LIGHT
#endif

/* Backward compatibility: after #8740 the RSA module offers functions to parse
 * and write RSA private/public keys without relying on the PK one. Of course
 * this needs ASN1 support to do so, so we enable it here. */
#if defined(MBEDTLS_RSA_C)
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#endif

/* MBEDTLS_PK_PARSE_EC_COMPRESSED is introduced in Mbed TLS version 3.5, while
 * in previous version compressed points were automatically supported as long
 * as PK_PARSE_C and ECP_C were enabled. As a consequence, for backward
 * compatibility, we auto-enable PK_PARSE_EC_COMPRESSED when these conditions
 * are met. */
#if defined(MBEDTLS_PK_PARSE_C) && defined(MBEDTLS_ECP_C)
#define MBEDTLS_PK_PARSE_EC_COMPRESSED
#endif

/* Helper symbol to state that there is support for ECDH, either through
 * library implementation (ECDH_C) or through PSA. */
#if (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_ALG_ECDH)) || \
    (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_ECDH_C))
#define MBEDTLS_CAN_ECDH
#endif

/* PK module can achieve ECDSA functionalities by means of either software
 * implementations (ECDSA_C) or through a PSA driver. The following defines
 * are meant to list these capabilities in a general way which abstracts how
 * they are implemented under the hood. */
#if !defined(MBEDTLS_USE_PSA_CRYPTO)
#if defined(MBEDTLS_ECDSA_C)
#define MBEDTLS_PK_CAN_ECDSA_SIGN
#define MBEDTLS_PK_CAN_ECDSA_VERIFY
#endif /* MBEDTLS_ECDSA_C */
#else /* MBEDTLS_USE_PSA_CRYPTO */
#if defined(PSA_WANT_ALG_ECDSA)
#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define MBEDTLS_PK_CAN_ECDSA_SIGN
#endif /* PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC */
#if defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
#define MBEDTLS_PK_CAN_ECDSA_VERIFY
#endif /* PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY */
#endif /* PSA_WANT_ALG_ECDSA */
#endif /* MBEDTLS_USE_PSA_CRYPTO */

#if defined(MBEDTLS_PK_CAN_ECDSA_VERIFY) || defined(MBEDTLS_PK_CAN_ECDSA_SIGN)
#define MBEDTLS_PK_CAN_ECDSA_SOME
#endif

/* Helpers to state that each key is supported either on the builtin or PSA side. */
#if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED) || defined(PSA_WANT_ECC_SECP_R1_521)
#define MBEDTLS_ECP_HAVE_SECP521R1
#endif
#if defined(MBEDTLS_ECP_DP_BP512R1_ENABLED) || defined(PSA_WANT_ECC_BRAINPOOL_P_R1_512)
#define MBEDTLS_ECP_HAVE_BP512R1
#endif
#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED) || defined(PSA_WANT_ECC_MONTGOMERY_448)
#define MBEDTLS_ECP_HAVE_CURVE448
#endif
#if defined(MBEDTLS_ECP_DP_BP384R1_ENABLED) || defined(PSA_WANT_ECC_BRAINPOOL_P_R1_384)
#define MBEDTLS_ECP_HAVE_BP384R1
#endif
#if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED) || defined(PSA_WANT_ECC_SECP_R1_384)
#define MBEDTLS_ECP_HAVE_SECP384R1
#endif
#if defined(MBEDTLS_ECP_DP_BP256R1_ENABLED) || defined(PSA_WANT_ECC_BRAINPOOL_P_R1_256)
#define MBEDTLS_ECP_HAVE_BP256R1
#endif
#if defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED) || defined(PSA_WANT_ECC_SECP_K1_256)
#define MBEDTLS_ECP_HAVE_SECP256K1
#endif
#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) || defined(PSA_WANT_ECC_SECP_R1_256)
#define MBEDTLS_ECP_HAVE_SECP256R1
#endif
#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED) || defined(PSA_WANT_ECC_MONTGOMERY_255)
#define MBEDTLS_ECP_HAVE_CURVE25519
#endif
#if defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED) || defined(PSA_WANT_ECC_SECP_K1_224)
#define MBEDTLS_ECP_HAVE_SECP224K1
#endif
#if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED) || defined(PSA_WANT_ECC_SECP_R1_224)
#define MBEDTLS_ECP_HAVE_SECP224R1
#endif
#if defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED) || defined(PSA_WANT_ECC_SECP_K1_192)
#define MBEDTLS_ECP_HAVE_SECP192K1
#endif
#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED) || defined(PSA_WANT_ECC_SECP_R1_192)
#define MBEDTLS_ECP_HAVE_SECP192R1
#endif

/* Helper symbol to state that the PK module has support for EC keys. This
 * can either be provided through the legacy ECP solution or through the
 * PSA friendly MBEDTLS_PK_USE_PSA_EC_DATA (see pk.h for its description). */
#if defined(MBEDTLS_ECP_C) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY))
#define MBEDTLS_PK_HAVE_ECC_KEYS
#endif /* MBEDTLS_PK_USE_PSA_EC_DATA || MBEDTLS_ECP_C */

/* Historically pkparse did not check the CBC padding when decrypting
 * a key. This was a bug, which is now fixed. As a consequence, pkparse
 * now needs PKCS7 padding support, but existing configurations might not
 * enable it, so we enable it here. */
#if defined(MBEDTLS_PK_PARSE_C) && defined(MBEDTLS_PKCS5_C) && defined(MBEDTLS_CIPHER_MODE_CBC)
#define MBEDTLS_CIPHER_PADDING_PKCS7
#endif

/* Backwards compatibility for some macros which were renamed to reflect that
 * they are related to Armv8, not aarch64. */
#if defined(MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT) && \
    !defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT)
#define MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT
#endif
#if defined(MBEDTLS_SHA256_USE_A64_CRYPTO_ONLY) && !defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY)
#define MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY
#endif

/* psa_util file features some ECDSA conversion functions, to convert between
 * legacy's ASN.1 DER format and PSA's raw one. */
#if (defined(MBEDTLS_PSA_CRYPTO_CLIENT) && \
    (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)))
#define MBEDTLS_PSA_UTIL_HAVE_ECDSA
#endif

/* Some internal helpers to determine which keys are available. */
#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_AES_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_KEY_TYPE_AES))
#define MBEDTLS_SSL_HAVE_AES
#endif
#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_ARIA_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_KEY_TYPE_ARIA))
#define MBEDTLS_SSL_HAVE_ARIA
#endif
#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_CAMELLIA_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_KEY_TYPE_CAMELLIA))
#define MBEDTLS_SSL_HAVE_CAMELLIA
#endif

/* Some internal helpers to determine which operation modes are available. */
#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_CIPHER_MODE_CBC)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_ALG_CBC_NO_PADDING))
#define MBEDTLS_SSL_HAVE_CBC
#endif

#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_GCM_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_ALG_GCM))
#define MBEDTLS_SSL_HAVE_GCM
#endif

#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_CCM_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_ALG_CCM))
#define MBEDTLS_SSL_HAVE_CCM
#endif

#if (!defined(MBEDTLS_USE_PSA_CRYPTO) && defined(MBEDTLS_CHACHAPOLY_C)) || \
    (defined(MBEDTLS_USE_PSA_CRYPTO) && defined(PSA_WANT_ALG_CHACHA20_POLY1305))
#define MBEDTLS_SSL_HAVE_CHACHAPOLY
#endif

#if defined(MBEDTLS_SSL_HAVE_GCM) || defined(MBEDTLS_SSL_HAVE_CCM) || \
    defined(MBEDTLS_SSL_HAVE_CHACHAPOLY)
#define MBEDTLS_SSL_HAVE_AEAD
#endif

#endif /* MBEDTLS_CONFIG_ADJUST_LEGACY_CRYPTO_H */


/* ===== Retained config_adjust_x509.h ===== */

/**
 * \file mbedtls/config_adjust_x509.h
 * \brief Adjust X.509 configuration
 *
 * This is an internal header. Do not include it directly.
 *
 * Automatically enable certain dependencies. Generally, MBEDTLS_xxx
 * configurations need to be explicitly enabled by the user: enabling
 * MBEDTLS_xxx_A but not MBEDTLS_xxx_B when A requires B results in a
 * compilation error. However, we do automatically enable certain options
 * in some circumstances. One case is if MBEDTLS_xxx_B is an internal option
 * used to identify parts of a module that are used by other module, and we
 * don't want to make the symbol MBEDTLS_xxx_B part of the public API.
 * Another case is if A didn't depend on B in earlier versions, and we
 * want to use B in A but we need to preserve backward compatibility with
 * configurations that explicitly activate MBEDTLS_xxx_A but not
 * MBEDTLS_xxx_B.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/config_adjust_x509.h.
 */

#ifndef MBEDTLS_CONFIG_ADJUST_X509_H
#define MBEDTLS_CONFIG_ADJUST_X509_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include mbedtls/config_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif /* */

#endif /* MBEDTLS_CONFIG_ADJUST_X509_H */


/* ===== Retained config_adjust_ssl.h ===== */

/**
 * \file mbedtls/config_adjust_ssl.h
 * \brief Adjust TLS configuration
 *
 * This is an internal header. Do not include it directly.
 *
 * Automatically enable certain dependencies. Generally, MBEDTLS_xxx
 * configurations need to be explicitly enabled by the user: enabling
 * MBEDTLS_xxx_A but not MBEDTLS_xxx_B when A requires B results in a
 * compilation error. However, we do automatically enable certain options
 * in some circumstances. One case is if MBEDTLS_xxx_B is an internal option
 * used to identify parts of a module that are used by other module, and we
 * don't want to make the symbol MBEDTLS_xxx_B part of the public API.
 * Another case is if A didn't depend on B in earlier versions, and we
 * want to use B in A but we need to preserve backward compatibility with
 * configurations that explicitly activate MBEDTLS_xxx_A but not
 * MBEDTLS_xxx_B.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/config_adjust_ssl.h.
 */

#ifndef MBEDTLS_CONFIG_ADJUST_SSL_H
#define MBEDTLS_CONFIG_ADJUST_SSL_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include mbedtls/config_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif /* */

/* The following blocks make it easier to disable all of TLS,
 * or of TLS 1.2 or 1.3 or DTLS, without having to manually disable all
 * key exchanges, options and extensions related to them. */

#if !defined(MBEDTLS_SSL_TLS_C)
#undef MBEDTLS_SSL_CLI_C
#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_PROTO_TLS1_3
#undef MBEDTLS_SSL_PROTO_TLS1_2
#undef MBEDTLS_SSL_PROTO_DTLS
#endif

#if !(defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_SESSION_TICKETS))
#undef MBEDTLS_SSL_TICKET_C
#endif

#if !defined(MBEDTLS_SSL_PROTO_DTLS)
#undef MBEDTLS_SSL_DTLS_ANTI_REPLAY
#undef MBEDTLS_SSL_DTLS_CONNECTION_ID
#undef MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT
#undef MBEDTLS_SSL_DTLS_HELLO_VERIFY
#undef MBEDTLS_SSL_DTLS_SRTP
#undef MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#endif

#if !defined(MBEDTLS_SSL_PROTO_TLS1_2)
#undef MBEDTLS_SSL_ENCRYPT_THEN_MAC
#undef MBEDTLS_SSL_EXTENDED_MASTER_SECRET
#undef MBEDTLS_SSL_RENEGOTIATION
#undef MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED
#endif

#if !defined(MBEDTLS_SSL_PROTO_TLS1_3)
#undef MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
#undef MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
#undef MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED
#undef MBEDTLS_SSL_EARLY_DATA
#undef MBEDTLS_SSL_RECORD_SIZE_LIMIT
#endif

#if defined(MBEDTLS_SSL_PROTO_TLS1_2) && \
    (defined(MBEDTLS_ECDH_C) || defined(MBEDTLS_ECDSA_C) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED))
#define MBEDTLS_SSL_TLS1_2_SOME_ECC
#endif

#endif /* MBEDTLS_CONFIG_ADJUST_SSL_H */


/* Indicate that all configuration symbols are set,
 * even the ones that are calculated programmatically.
 * It is now safe to query the configuration (to check it, to size buffers,
 * etc.).
 */
#define MBEDTLS_CONFIG_IS_FINALIZED

/* ===== Retained check_config.h ===== */

/**
 * \file check_config.h
 *
 * \brief Consistency checks for configuration options
 *
 * This is an internal header. Do not include it directly.
 *
 * This header is included automatically by all public Mbed TLS headers
 * (via mbedtls/build_info.h). Do not include it directly in a configuration
 * file such as mbedtls/mbedtls_config.h or #MBEDTLS_USER_CONFIG_FILE!
 * It would run at the wrong time due to missing derived symbols.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/check_config.h.
 */

#ifndef MBEDTLS_CHECK_CONFIG_H
#define MBEDTLS_CHECK_CONFIG_H

/* *INDENT-OFF* */

#if !defined(MBEDTLS_CONFIG_IS_FINALIZED)
#warning "Do not include mbedtls/check_config.h manually! " \
         "This may cause spurious errors. " \
         "It is included automatically at the right point since Mbed TLS 3.0."
#endif /* !MBEDTLS_CONFIG_IS_FINALIZED */

/*
 * We assume CHAR_BIT is 8 in many places. In practice, this is true on our
 * target platforms, so not an issue, but let's just be extra sure.
 */
#include <limits.h>
#if CHAR_BIT != 8
#error "Mbed TLS requires a platform with 8-bit chars"
#endif

#include <stdint.h>

#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER <= 1900)
#if !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_C is required on Windows"
#endif
/* See auto-enabling SNPRINTF_ALT and VSNPRINTF_ALT
 * in * config_adjust_legacy_crypto.h */
#endif /* _MINGW32__ || (_MSC_VER && (_MSC_VER <= 1900)) */

#if defined(TARGET_LIKE_MBED) && defined(MBEDTLS_NET_C)
#error "The NET module is not available for mbed OS - please use the network functions provided by Mbed OS"
#endif

#if defined(MBEDTLS_DEPRECATED_WARNING) && \
    !defined(__GNUC__) && !defined(__clang__)
#error "MBEDTLS_DEPRECATED_WARNING only works with GCC and Clang"
#endif

#if defined(MBEDTLS_HAVE_TIME_DATE) && !defined(MBEDTLS_HAVE_TIME)
#error "MBEDTLS_HAVE_TIME_DATE without MBEDTLS_HAVE_TIME does not make sense"
#endif

/* Limitations on ECC key types acceleration: if we have any of `PUBLIC_KEY`,
 * `KEY_PAIR_BASIC`, `KEY_PAIR_IMPORT`, `KEY_PAIR_EXPORT` then we must have
 * all 4 of them.
 */
#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY) || \
    defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC) || \
    defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY) || \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC) || \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#error "Unsupported partial support for ECC key type acceleration, see docs/driver-only-builds.md"
#endif /* not all of public, basic, import, export */
#endif /* one of public, basic, import, export */

/* Limitations on ECC curves acceleration: partial curve acceleration is only
 * supported with crypto excluding PK, X.509 or TLS.
 * Note: no need to check X.509 as it depends on PK. */
#if defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_256) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_384) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_512) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_255) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_448) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_192) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_224) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_256) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_192) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_224) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_256) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_384) || \
    defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_521)
#if defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES)
#if defined(MBEDTLS_PK_C) || \
    defined(MBEDTLS_SSL_TLS_C)
#error "Unsupported partial support for ECC curves acceleration, see docs/driver-only-builds.md"
#endif /* modules beyond what's supported */
#endif /* not all curves accelerated */
#endif /* some curve accelerated */

#if defined(MBEDTLS_CTR_DRBG_C) && !(defined(MBEDTLS_AES_C) || \
    (defined(MBEDTLS_PSA_CRYPTO_CLIENT) && defined(PSA_WANT_KEY_TYPE_AES) && \
    defined(PSA_WANT_ALG_ECB_NO_PADDING)))
#error "MBEDTLS_CTR_DRBG_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_DHM_C) && !defined(MBEDTLS_BIGNUM_C)
#error "MBEDTLS_DHM_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_CMAC_C) && \
    ( !defined(MBEDTLS_CIPHER_C ) || ( !defined(MBEDTLS_AES_C) && !defined(MBEDTLS_DES_C) ) )
#error "MBEDTLS_CMAC_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_NIST_KW_C) && \
    ( !defined(MBEDTLS_AES_C) || !defined(MBEDTLS_CIPHER_C) )
#error "MBEDTLS_NIST_KW_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT) && defined(MBEDTLS_PSA_CRYPTO_CONFIG)
#if defined(PSA_WANT_ALG_CBC_NO_PADDING)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and PSA_WANT_ALG_CBC_NO_PADDING cannot be defined simultaneously"
#endif
#if defined(PSA_WANT_ALG_CBC_PKCS7)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and PSA_WANT_ALG_CBC_PKCS7 cannot be defined simultaneously"
#endif
#if defined(PSA_WANT_ALG_ECB_NO_PADDING)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and PSA_WANT_ALG_ECB_NO_PADDING cannot be defined simultaneously"
#endif
#if defined(PSA_WANT_KEY_TYPE_DES)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and PSA_WANT_KEY_TYPE_DES cannot be defined simultaneously"
#endif
#endif

#if defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
#if defined(MBEDTLS_CIPHER_MODE_CBC)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and MBEDTLS_CIPHER_MODE_CBC cannot be defined simultaneously"
#endif
#if defined(MBEDTLS_CIPHER_MODE_XTS)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and MBEDTLS_CIPHER_MODE_XTS cannot be defined simultaneously"
#endif
#if defined(MBEDTLS_DES_C)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and MBEDTLS_DES_C cannot be defined simultaneously"
#endif
#if defined(MBEDTLS_NIST_KW_C)
#error "MBEDTLS_BLOCK_CIPHER_NO_DECRYPT and MBEDTLS_NIST_KW_C cannot be defined simultaneously"
#endif
#endif

#if defined(MBEDTLS_ECDH_C) && !defined(MBEDTLS_ECP_C)
#error "MBEDTLS_ECDH_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECDSA_C) &&            \
    ( !defined(MBEDTLS_ECP_C) ||           \
      !( defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED) || \
         defined(MBEDTLS_ECP_DP_BP256R1_ENABLED) ||   \
         defined(MBEDTLS_ECP_DP_BP384R1_ENABLED) ||   \
         defined(MBEDTLS_ECP_DP_BP512R1_ENABLED) ) || \
      !defined(MBEDTLS_ASN1_PARSE_C) ||    \
      !defined(MBEDTLS_ASN1_WRITE_C) )
#error "MBEDTLS_ECDSA_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PK_C) && defined(MBEDTLS_USE_PSA_CRYPTO)
#if defined(MBEDTLS_PK_CAN_ECDSA_SIGN) && !defined(MBEDTLS_ASN1_WRITE_C)
#error "MBEDTLS_PK_C with MBEDTLS_USE_PSA_CRYPTO needs MBEDTLS_ASN1_WRITE_C for ECDSA signature"
#endif
#if defined(MBEDTLS_PK_CAN_ECDSA_VERIFY) && !defined(MBEDTLS_ASN1_PARSE_C)
#error "MBEDTLS_PK_C with MBEDTLS_USE_PSA_CRYPTO needs MBEDTLS_ASN1_PARSE_C for ECDSA verification"
#endif
#endif /* MBEDTLS_PK_C && MBEDTLS_USE_PSA_CRYPTO */

#if defined(MBEDTLS_ECJPAKE_C) && \
    !defined(MBEDTLS_ECP_C)
#error "MBEDTLS_ECJPAKE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_RESTARTABLE)           && \
    ( defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT) || \
      defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)     || \
      defined(MBEDTLS_ECDSA_SIGN_ALT)          || \
      defined(MBEDTLS_ECDSA_VERIFY_ALT)        || \
      defined(MBEDTLS_ECDSA_GENKEY_ALT)        || \
      defined(MBEDTLS_ECP_INTERNAL_ALT)        || \
      defined(MBEDTLS_ECP_ALT) )
#error "MBEDTLS_ECP_RESTARTABLE defined, but it cannot coexist with an alternative ECP implementation"
#endif

#if defined(MBEDTLS_ECP_RESTARTABLE)           && \
    !defined(MBEDTLS_ECP_C)
#error "MBEDTLS_ECP_RESTARTABLE defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECDSA_DETERMINISTIC) && !defined(MBEDTLS_HMAC_DRBG_C)
#error "MBEDTLS_ECDSA_DETERMINISTIC defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_LIGHT) && ( !defined(MBEDTLS_BIGNUM_C) || (    \
    !defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)   &&                  \
    !defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)   &&                  \
    !defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)   &&                  \
    !defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED) &&                  \
    !defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED) &&                 \
    !defined(MBEDTLS_ECP_DP_CURVE448_ENABLED) ) )
#error "MBEDTLS_ECP_C defined (or a subset enabled), but not all prerequisites"
#endif

#if defined(MBEDTLS_ENTROPY_C) && \
    !(defined(MBEDTLS_MD_CAN_SHA512) || defined(MBEDTLS_MD_CAN_SHA256))
#error "MBEDTLS_ENTROPY_C defined, but not all prerequisites"
#endif
#if defined(MBEDTLS_ENTROPY_C) && \
    defined(MBEDTLS_CTR_DRBG_ENTROPY_LEN) && (MBEDTLS_CTR_DRBG_ENTROPY_LEN > 64)
#error "MBEDTLS_CTR_DRBG_ENTROPY_LEN value too high"
#endif
#if defined(MBEDTLS_ENTROPY_C) &&                                            \
    (defined(MBEDTLS_ENTROPY_FORCE_SHA256) || !defined(MBEDTLS_MD_CAN_SHA512)) \
    && defined(MBEDTLS_CTR_DRBG_ENTROPY_LEN) && (MBEDTLS_CTR_DRBG_ENTROPY_LEN > 32)
#error "MBEDTLS_CTR_DRBG_ENTROPY_LEN value too high"
#endif
#if defined(MBEDTLS_ENTROPY_C) && \
    defined(MBEDTLS_ENTROPY_FORCE_SHA256) && !defined(MBEDTLS_MD_CAN_SHA256)
#error "MBEDTLS_ENTROPY_FORCE_SHA256 defined, but not all prerequisites"
#endif

#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
#define MBEDTLS_HAS_MEMSAN // #undef at the end of this paragraph
#endif
#endif
#if defined(MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN) &&  !defined(MBEDTLS_HAS_MEMSAN)
#error "MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN requires building with MemorySanitizer"
#endif
#if defined(MBEDTLS_HAS_MEMSAN) && defined(MBEDTLS_HAVE_ASM)
#error "MemorySanitizer does not support assembly implementation"
#endif
#undef MBEDTLS_HAS_MEMSAN // temporary macro defined above

#if defined(MBEDTLS_CCM_C) && \
    !(defined(MBEDTLS_CCM_GCM_CAN_AES) || defined(MBEDTLS_CCM_GCM_CAN_ARIA) || \
    defined(MBEDTLS_CCM_GCM_CAN_CAMELLIA))
#error "MBEDTLS_CCM_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_GCM_C) && \
    !(defined(MBEDTLS_CCM_GCM_CAN_AES) || defined(MBEDTLS_CCM_GCM_CAN_ARIA) || \
    defined(MBEDTLS_CCM_GCM_CAN_CAMELLIA))
#error "MBEDTLS_GCM_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_CHACHAPOLY_C) && !defined(MBEDTLS_CHACHA20_C)
#error "MBEDTLS_CHACHAPOLY_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_CHACHAPOLY_C) && !defined(MBEDTLS_POLY1305_C)
#error "MBEDTLS_CHACHAPOLY_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_RANDOMIZE_JAC_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_ADD_MIXED_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_ADD_MIXED_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_DOUBLE_JAC_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_DOUBLE_JAC_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_NORMALIZE_JAC_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_RANDOMIZE_MXZ_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_RANDOMIZE_MXZ_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_NORMALIZE_MXZ_ALT) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_NORMALIZE_MXZ_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ECP_NO_FALLBACK) && !defined(MBEDTLS_ECP_INTERNAL_ALT)
#error "MBEDTLS_ECP_NO_FALLBACK defined, but no alternative implementation enabled"
#endif

#if defined(MBEDTLS_HKDF_C) && !defined(MBEDTLS_MD_C)
#error "MBEDTLS_HKDF_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_HMAC_DRBG_C) && !defined(MBEDTLS_MD_C)
#error "MBEDTLS_HMAC_DRBG_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED) &&                 \
    ( !defined(MBEDTLS_CAN_ECDH) ||                                       \
      !defined(MBEDTLS_PK_CAN_ECDSA_SIGN) ||                                \
      !defined(MBEDTLS_X509_CRT_PARSE_C) )
#error "MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED) &&                 \
    ( !defined(MBEDTLS_CAN_ECDH) || !defined(MBEDTLS_RSA_C) ||          \
      !defined(MBEDTLS_X509_CRT_PARSE_C) )
#error "MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED) && !defined(MBEDTLS_DHM_C)
#error "MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED) &&                     \
    !defined(MBEDTLS_CAN_ECDH)
#error "MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED) &&                   \
    ( !defined(MBEDTLS_DHM_C) || !defined(MBEDTLS_RSA_C) ||           \
      !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_PKCS1_V15) )
#error "MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED) &&                 \
    ( !defined(MBEDTLS_CAN_ECDH) || !defined(MBEDTLS_RSA_C) ||          \
      !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_PKCS1_V15) )
#error "MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED) &&                \
    ( !defined(MBEDTLS_CAN_ECDH) ||                                       \
      !defined(MBEDTLS_PK_CAN_ECDSA_SIGN) ||                                \
      !defined(MBEDTLS_X509_CRT_PARSE_C) )
#error "MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED) &&                   \
    ( !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
      !defined(MBEDTLS_PKCS1_V15) )
#error "MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED) &&                       \
    ( !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
      !defined(MBEDTLS_PKCS1_V15) )
#error "MBEDTLS_KEY_EXCHANGE_RSA_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_USE_PSA_CRYPTO)
#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED) &&    \
    ( !defined(PSA_WANT_ALG_JPAKE) ||                   \
      !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC) || \
      !defined(PSA_WANT_ECC_SECP_R1_256) )
#error "MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED defined, but not all prerequisites"
#endif
#else /* MBEDTLS_USE_PSA_CRYPTO */
#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED) &&    \
    ( !defined(MBEDTLS_ECJPAKE_C) ||                    \
      !defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) )
#error "MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED defined, but not all prerequisites"
#endif
#endif /* MBEDTLS_USE_PSA_CRYPTO */

/* Use of EC J-PAKE in TLS requires SHA-256. */
#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED) &&                    \
    !defined(MBEDTLS_MD_CAN_SHA256)
#error "MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED) &&        \
    !defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE) &&            \
    !defined(MBEDTLS_MD_CAN_SHA256) &&                        \
    !defined(MBEDTLS_MD_CAN_SHA512) &&                        \
    !defined(MBEDTLS_MD_CAN_SHA1)
#error "!MBEDTLS_SSL_KEEP_PEER_CERTIFICATE requires SHA-512, SHA-256 or SHA-1".
#endif

#if defined(MBEDTLS_MD_C) && \
    !defined(MBEDTLS_MD_CAN_MD5) && \
    !defined(MBEDTLS_MD_CAN_RIPEMD160) && \
    !defined(MBEDTLS_MD_CAN_SHA1) && \
    !defined(MBEDTLS_MD_CAN_SHA224) && \
    !defined(MBEDTLS_MD_CAN_SHA256) && \
    !defined(MBEDTLS_MD_CAN_SHA384) && \
    !defined(MBEDTLS_MD_CAN_SHA512) && \
    !defined(MBEDTLS_MD_CAN_SHA3_224) && \
    !defined(MBEDTLS_MD_CAN_SHA3_256) && \
    !defined(MBEDTLS_MD_CAN_SHA3_384) && \
    !defined(MBEDTLS_MD_CAN_SHA3_512)
#error "MBEDTLS_MD_C defined, but no hash algorithm"
#endif

#if defined(MBEDTLS_LMS_C) &&                                          \
    ! ( defined(MBEDTLS_PSA_CRYPTO_CLIENT) && defined(PSA_WANT_ALG_SHA_256) )
#error "MBEDTLS_LMS_C requires MBEDTLS_PSA_CRYPTO_C and PSA_WANT_ALG_SHA_256"
#endif

#if defined(MBEDTLS_LMS_PRIVATE) &&                                    \
    ( !defined(MBEDTLS_LMS_C) )
#error "MBEDTLS_LMS_PRIVATE requires MBEDTLS_LMS_C"
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) &&                          \
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_PLATFORM_MEMORY) )
#error "MBEDTLS_MEMORY_BUFFER_ALLOC_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_MEMORY_BACKTRACE) && !defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#error "MBEDTLS_MEMORY_BACKTRACE defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_MEMORY_DEBUG) && !defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#error "MBEDTLS_MEMORY_DEBUG defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PEM_PARSE_C) && !defined(MBEDTLS_BASE64_C)
#error "MBEDTLS_PEM_PARSE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PEM_WRITE_C) && !defined(MBEDTLS_BASE64_C)
#error "MBEDTLS_PEM_WRITE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PK_C) && \
    !defined(MBEDTLS_RSA_C) && !defined(MBEDTLS_PK_HAVE_ECC_KEYS)
#error "MBEDTLS_PK_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PK_PARSE_C) && \
    (!defined(MBEDTLS_ASN1_PARSE_C) || \
     !defined(MBEDTLS_OID_C)        || \
     !defined(MBEDTLS_PK_C))
#error "MBEDTLS_PK_PARSE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PK_WRITE_C) && \
    (!defined(MBEDTLS_ASN1_WRITE_C) || \
     !defined(MBEDTLS_OID_C)        || \
     !defined(MBEDTLS_PK_C))
#error "MBEDTLS_PK_WRITE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_EXIT_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_EXIT_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_EXIT_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_EXIT_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_EXIT_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_EXIT) ||\
        defined(MBEDTLS_PLATFORM_EXIT_ALT) )
#error "MBEDTLS_PLATFORM_EXIT_MACRO and MBEDTLS_PLATFORM_STD_EXIT/MBEDTLS_PLATFORM_EXIT_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_SETBUF_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_SETBUF_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_SETBUF_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_SETBUF_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_SETBUF_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_SETBUF) ||\
        defined(MBEDTLS_PLATFORM_SETBUF_ALT) )
#error "MBEDTLS_PLATFORM_SETBUF_MACRO and MBEDTLS_PLATFORM_STD_SETBUF/MBEDTLS_PLATFORM_SETBUF_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_TIME_ALT) &&\
    ( !defined(MBEDTLS_PLATFORM_C) ||\
        !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_TIME_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_TIME_MACRO) &&\
    ( !defined(MBEDTLS_PLATFORM_C) ||\
        !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_TIME_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_MS_TIME_TYPE_MACRO) &&\
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_MS_TIME_TYPE_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_MS_TIME_ALT)   && \
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_MS_TIME_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_TIME_TYPE_MACRO) &&\
    ( !defined(MBEDTLS_PLATFORM_C) ||\
        !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_TIME_TYPE_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_TIME_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_TIME) ||\
        defined(MBEDTLS_PLATFORM_TIME_ALT) )
#error "MBEDTLS_PLATFORM_TIME_MACRO and MBEDTLS_PLATFORM_STD_TIME/MBEDTLS_PLATFORM_TIME_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_TIME_TYPE_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_TIME) ||\
        defined(MBEDTLS_PLATFORM_TIME_ALT) )
#error "MBEDTLS_PLATFORM_TIME_TYPE_MACRO and MBEDTLS_PLATFORM_STD_TIME/MBEDTLS_PLATFORM_TIME_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_FPRINTF_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_FPRINTF_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_FPRINTF_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_FPRINTF_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_FPRINTF_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_FPRINTF) ||\
        defined(MBEDTLS_PLATFORM_FPRINTF_ALT) )
#error "MBEDTLS_PLATFORM_FPRINTF_MACRO and MBEDTLS_PLATFORM_STD_FPRINTF/MBEDTLS_PLATFORM_FPRINTF_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_FREE_MACRO) &&\
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_PLATFORM_MEMORY) )
#error "MBEDTLS_PLATFORM_FREE_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_FREE_MACRO) &&\
    defined(MBEDTLS_PLATFORM_STD_FREE)
#error "MBEDTLS_PLATFORM_FREE_MACRO and MBEDTLS_PLATFORM_STD_FREE cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_FREE_MACRO) && !defined(MBEDTLS_PLATFORM_CALLOC_MACRO)
#error "MBEDTLS_PLATFORM_CALLOC_MACRO must be defined if MBEDTLS_PLATFORM_FREE_MACRO is"
#endif

#if defined(MBEDTLS_PLATFORM_CALLOC_MACRO) &&\
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_PLATFORM_MEMORY) )
#error "MBEDTLS_PLATFORM_CALLOC_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_CALLOC_MACRO) &&\
    defined(MBEDTLS_PLATFORM_STD_CALLOC)
#error "MBEDTLS_PLATFORM_CALLOC_MACRO and MBEDTLS_PLATFORM_STD_CALLOC cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_CALLOC_MACRO) && !defined(MBEDTLS_PLATFORM_FREE_MACRO)
#error "MBEDTLS_PLATFORM_FREE_MACRO must be defined if MBEDTLS_PLATFORM_CALLOC_MACRO is"
#endif

#if defined(MBEDTLS_PLATFORM_MEMORY) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_MEMORY defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_PRINTF_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_PRINTF_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_PRINTF_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_PRINTF_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_PRINTF_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_PRINTF) ||\
        defined(MBEDTLS_PLATFORM_PRINTF_ALT) )
#error "MBEDTLS_PLATFORM_PRINTF_MACRO and MBEDTLS_PLATFORM_STD_PRINTF/MBEDTLS_PLATFORM_PRINTF_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_SNPRINTF_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_SNPRINTF_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_SNPRINTF_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_SNPRINTF_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_SNPRINTF_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_SNPRINTF) ||\
        defined(MBEDTLS_PLATFORM_SNPRINTF_ALT) )
#error "MBEDTLS_PLATFORM_SNPRINTF_MACRO and MBEDTLS_PLATFORM_STD_SNPRINTF/MBEDTLS_PLATFORM_SNPRINTF_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_VSNPRINTF_ALT) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_VSNPRINTF_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_VSNPRINTF_MACRO) && !defined(MBEDTLS_PLATFORM_C)
#error "MBEDTLS_PLATFORM_VSNPRINTF_MACRO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_VSNPRINTF_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_VSNPRINTF) ||\
        defined(MBEDTLS_PLATFORM_VSNPRINTF_ALT) )
#error "MBEDTLS_PLATFORM_VSNPRINTF_MACRO and MBEDTLS_PLATFORM_STD_VSNPRINTF/MBEDTLS_PLATFORM_VSNPRINTF_ALT cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_STD_MEM_HDR) &&\
    !defined(MBEDTLS_PLATFORM_NO_STD_FUNCTIONS)
#error "MBEDTLS_PLATFORM_STD_MEM_HDR defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_CALLOC) && !defined(MBEDTLS_PLATFORM_MEMORY)
#error "MBEDTLS_PLATFORM_STD_CALLOC defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_FREE) && !defined(MBEDTLS_PLATFORM_MEMORY)
#error "MBEDTLS_PLATFORM_STD_FREE defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_EXIT) &&\
    !defined(MBEDTLS_PLATFORM_EXIT_ALT)
#error "MBEDTLS_PLATFORM_STD_EXIT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_TIME) &&\
    ( !defined(MBEDTLS_PLATFORM_TIME_ALT) ||\
        !defined(MBEDTLS_HAVE_TIME) )
#error "MBEDTLS_PLATFORM_STD_TIME defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_FPRINTF) &&\
    !defined(MBEDTLS_PLATFORM_FPRINTF_ALT)
#error "MBEDTLS_PLATFORM_STD_FPRINTF defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_PRINTF) &&\
    !defined(MBEDTLS_PLATFORM_PRINTF_ALT)
#error "MBEDTLS_PLATFORM_STD_PRINTF defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_SNPRINTF) &&\
    !defined(MBEDTLS_PLATFORM_SNPRINTF_ALT)
#error "MBEDTLS_PLATFORM_STD_SNPRINTF defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_ENTROPY_NV_SEED) &&\
    ( !defined(MBEDTLS_PLATFORM_C) || !defined(MBEDTLS_ENTROPY_C) )
#error "MBEDTLS_ENTROPY_NV_SEED defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_NV_SEED_ALT) &&\
    !defined(MBEDTLS_ENTROPY_NV_SEED)
#error "MBEDTLS_PLATFORM_NV_SEED_ALT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_NV_SEED_READ) &&\
    !defined(MBEDTLS_PLATFORM_NV_SEED_ALT)
#error "MBEDTLS_PLATFORM_STD_NV_SEED_READ defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_STD_NV_SEED_WRITE) &&\
    !defined(MBEDTLS_PLATFORM_NV_SEED_ALT)
#error "MBEDTLS_PLATFORM_STD_NV_SEED_WRITE defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PLATFORM_NV_SEED_READ_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_NV_SEED_READ) ||\
      defined(MBEDTLS_PLATFORM_NV_SEED_ALT) )
#error "MBEDTLS_PLATFORM_NV_SEED_READ_MACRO and MBEDTLS_PLATFORM_STD_NV_SEED_READ cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO) &&\
    ( defined(MBEDTLS_PLATFORM_STD_NV_SEED_WRITE) ||\
      defined(MBEDTLS_PLATFORM_NV_SEED_ALT) )
#error "MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO and MBEDTLS_PLATFORM_STD_NV_SEED_WRITE cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PSA_CRYPTO_C) &&                                    \
    !( ( ( defined(MBEDTLS_CTR_DRBG_C) || defined(MBEDTLS_HMAC_DRBG_C) ) && \
         defined(MBEDTLS_ENTROPY_C) ) ||                                \
       defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG) )
#error "MBEDTLS_PSA_CRYPTO_C defined, but not all prerequisites (missing RNG)"
#endif

#if defined(MBEDTLS_PSA_CRYPTO_C) && defined(PSA_HAVE_SOFT_BLOCK_MODE) && \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER) && !defined(MBEDTLS_CIPHER_C)
#error "MBEDTLS_PSA_CRYPTO_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PSA_CRYPTO_SPM) && !defined(MBEDTLS_PSA_CRYPTO_C)
#error "MBEDTLS_PSA_CRYPTO_SPM defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PSA_CRYPTO_SE_C) &&    \
    ! ( defined(MBEDTLS_PSA_CRYPTO_C) && \
        defined(MBEDTLS_PSA_CRYPTO_STORAGE_C) )
#error "MBEDTLS_PSA_CRYPTO_SE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
#if defined(MBEDTLS_DEPRECATED_REMOVED)
#error "MBEDTLS_PSA_CRYPTO_SE_C is deprecated and will be removed in a future version of Mbed TLS"
#elif defined(MBEDTLS_DEPRECATED_WARNING)
#warning "MBEDTLS_PSA_CRYPTO_SE_C is deprecated and will be removed in a future version of Mbed TLS"
#endif
#endif /* MBEDTLS_PSA_CRYPTO_SE_C */

#if defined(MBEDTLS_PSA_CRYPTO_STORAGE_C) &&            \
    ! defined(MBEDTLS_PSA_CRYPTO_C)
#error "MBEDTLS_PSA_CRYPTO_STORAGE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PSA_INJECT_ENTROPY) &&      \
    !( defined(MBEDTLS_PSA_CRYPTO_STORAGE_C) && \
       defined(MBEDTLS_ENTROPY_NV_SEED) )
#error "MBEDTLS_PSA_INJECT_ENTROPY defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_PSA_INJECT_ENTROPY) &&              \
    !defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
#error "MBEDTLS_PSA_INJECT_ENTROPY is not compatible with actual entropy sources"
#endif

#if defined(MBEDTLS_PSA_INJECT_ENTROPY) &&              \
    defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
#error "MBEDTLS_PSA_INJECT_ENTROPY is not compatible with MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG"
#endif

#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC) &&           \
    defined(MBEDTLS_PSA_STATIC_KEY_SLOTS)
#error "MBEDTLS_PSA_KEY_STORE_DYNAMIC and MBEDTLS_PSA_STATIC_KEY_SLOTS cannot be defined simultaneously"
#endif

#if defined(MBEDTLS_PSA_ITS_FILE_C) && \
    !defined(MBEDTLS_FS_IO)
#error "MBEDTLS_PSA_ITS_FILE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_RSA_C) && ( !defined(MBEDTLS_BIGNUM_C) ||         \
    !defined(MBEDTLS_OID_C) )
#error "MBEDTLS_RSA_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_RSA_C) && ( !defined(MBEDTLS_PKCS1_V21) &&         \
    !defined(MBEDTLS_PKCS1_V15) )
#error "MBEDTLS_RSA_C defined, but none of the PKCS1 versions enabled"
#endif

#if defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT) &&                        \
    ( !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_PKCS1_V21) )
#error "MBEDTLS_X509_RSASSA_PSS_SUPPORT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SHA512_USE_A64_CRYPTO_IF_PRESENT) && \
    defined(MBEDTLS_SHA512_USE_A64_CRYPTO_ONLY)
#error "Must only define one of MBEDTLS_SHA512_USE_A64_CRYPTO_*"
#endif

#if defined(MBEDTLS_SHA512_USE_A64_CRYPTO_IF_PRESENT) || \
    defined(MBEDTLS_SHA512_USE_A64_CRYPTO_ONLY)
#if !defined(MBEDTLS_SHA512_C)
#error "MBEDTLS_SHA512_USE_A64_CRYPTO_* defined without MBEDTLS_SHA512_C"
#endif
#if defined(MBEDTLS_SHA512_ALT) || defined(MBEDTLS_SHA512_PROCESS_ALT)
#error "MBEDTLS_SHA512_*ALT can't be used with MBEDTLS_SHA512_USE_A64_CRYPTO_*"
#endif

#endif /* MBEDTLS_SHA512_USE_A64_CRYPTO_IF_PRESENT || MBEDTLS_SHA512_USE_A64_CRYPTO_ONLY */

#if defined(MBEDTLS_SHA512_USE_A64_CRYPTO_ONLY) && !defined(__aarch64__)
#error "MBEDTLS_SHA512_USE_A64_CRYPTO_ONLY defined on non-Aarch64 system"
#endif

#if defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT) && \
    defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY)
#error "Must only define one of MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_*"
#endif

#if defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT) || \
    defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY)
#if !defined(MBEDTLS_SHA256_C)
#error "MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_* defined without MBEDTLS_SHA256_C"
#endif
#if defined(MBEDTLS_SHA256_ALT) || defined(MBEDTLS_SHA256_PROCESS_ALT)
#error "MBEDTLS_SHA256_*ALT can't be used with MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_*"
#endif

#endif

#if defined(MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY) && !defined(MBEDTLS_ARCH_IS_ARMV8_A)
#error "MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY defined on non-Armv8-A system"
#endif

/* TLS 1.3 requires separate HKDF parts from PSA,
 * and at least one ciphersuite, so at least SHA-256 or SHA-384
 * from PSA to use with HKDF.
 *
 * Note: for dependencies common with TLS 1.2 (running handshake hash),
 * see MBEDTLS_SSL_TLS_C. */
#if defined(MBEDTLS_SSL_PROTO_TLS1_3) && \
    !(defined(MBEDTLS_PSA_CRYPTO_CLIENT) && \
      defined(PSA_WANT_ALG_HKDF_EXTRACT) && \
      defined(PSA_WANT_ALG_HKDF_EXPAND) && \
      (defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA_384)))
#error "MBEDTLS_SSL_PROTO_TLS1_3 defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
#if !( (defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)) && \
       defined(MBEDTLS_X509_CRT_PARSE_C) && \
       ( defined(MBEDTLS_PK_CAN_ECDSA_SIGN) || defined(MBEDTLS_PKCS1_V21) ) )
#error "MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED defined, but not all prerequisites"
#endif
#endif

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED)
#if !( defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH) )
#error "MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED defined, but not all prerequisites"
#endif
#endif

/*
 * The current implementation of TLS 1.3 requires MBEDTLS_SSL_KEEP_PEER_CERTIFICATE.
 */
#if defined(MBEDTLS_SSL_PROTO_TLS1_3) && !defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
#error "MBEDTLS_SSL_PROTO_TLS1_3 defined without MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
#endif

#if defined(MBEDTLS_SSL_PROTO_TLS1_2) &&                                    \
    !(defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED) ||                          \
      defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED) ||                      \
      defined(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED) ||                    \
      defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED) ||                  \
      defined(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED) ||                     \
      defined(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED) ||                   \
      defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED) ||                          \
      defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED) ||                      \
      defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED) ||                      \
      defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED) ||                    \
      defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED) )
#error "One or more versions of the TLS protocol are enabled " \
        "but no key exchange methods defined with MBEDTLS_KEY_EXCHANGE_xxxx"
#endif

#if defined(MBEDTLS_SSL_EARLY_DATA) && \
    ( !defined(MBEDTLS_SSL_SESSION_TICKETS) || \
      ( !defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED) && \
        !defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED) ) )
#error "MBEDTLS_SSL_EARLY_DATA  defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_EARLY_DATA) && defined(MBEDTLS_SSL_SRV_C) && \
    defined(MBEDTLS_SSL_MAX_EARLY_DATA_SIZE) &&                      \
        ((MBEDTLS_SSL_MAX_EARLY_DATA_SIZE < 0) ||                    \
         (MBEDTLS_SSL_MAX_EARLY_DATA_SIZE > UINT32_MAX))
#error "MBEDTLS_SSL_MAX_EARLY_DATA_SIZE must be in the range(0..UINT32_MAX)"
#endif

#if defined(MBEDTLS_SSL_PROTO_DTLS)     && \
    !defined(MBEDTLS_SSL_PROTO_TLS1_2)
#error "MBEDTLS_SSL_PROTO_DTLS defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_CLI_C) && !defined(MBEDTLS_SSL_TLS_C)
#error "MBEDTLS_SSL_CLI_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE) && !defined(MBEDTLS_X509_CRT_PARSE_C)
#error "MBEDTLS_SSL_ASYNC_PRIVATE defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TLS_C) && !(defined(MBEDTLS_CIPHER_C) || \
    defined(MBEDTLS_USE_PSA_CRYPTO))
#error "MBEDTLS_SSL_TLS_C defined, but not all prerequisites"
#endif

/* TLS 1.2 and 1.3 require SHA-256 or SHA-384 (running handshake hash) */
#if defined(MBEDTLS_SSL_TLS_C)
#if defined(MBEDTLS_USE_PSA_CRYPTO)
#if !(defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA_384))
#error "MBEDTLS_SSL_TLS_C defined, but not all prerequisites"
#endif
#else /* MBEDTLS_USE_PSA_CRYPTO */
#if !defined(MBEDTLS_MD_C) || \
    !(defined(MBEDTLS_MD_CAN_SHA256) || defined(MBEDTLS_MD_CAN_SHA384))
#error "MBEDTLS_SSL_TLS_C defined, but not all prerequisites"
#endif
#endif /* MBEDTLS_USE_PSA_CRYPTO */
#endif /* MBEDTLS_SSL_TLS_C */

#if defined(MBEDTLS_SSL_SRV_C) && !defined(MBEDTLS_SSL_TLS_C)
#error "MBEDTLS_SSL_SRV_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TLS_C) && \
    !( defined(MBEDTLS_SSL_PROTO_TLS1_2) || defined(MBEDTLS_SSL_PROTO_TLS1_3) )
#error "MBEDTLS_SSL_TLS_C defined, but no protocols are active"
#endif

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY) && !defined(MBEDTLS_SSL_PROTO_DTLS)
#error "MBEDTLS_SSL_DTLS_HELLO_VERIFY  defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE) && \
    !defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
#error "MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE  defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY) &&                              \
    ( !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) )
#error "MBEDTLS_SSL_DTLS_ANTI_REPLAY  defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID) &&                              \
    ( !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) )
#error "MBEDTLS_SSL_DTLS_CONNECTION_ID  defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)            &&                 \
    defined(MBEDTLS_SSL_CID_IN_LEN_MAX) &&                 \
    MBEDTLS_SSL_CID_IN_LEN_MAX > 255
#error "MBEDTLS_SSL_CID_IN_LEN_MAX too large (max 255)"
#endif

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)            &&                  \
    defined(MBEDTLS_SSL_CID_OUT_LEN_MAX) &&                 \
    MBEDTLS_SSL_CID_OUT_LEN_MAX > 255
#error "MBEDTLS_SSL_CID_OUT_LEN_MAX too large (max 255)"
#endif

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT)     &&                 \
    !defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)
#error "MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT) && MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT != 0
#if defined(MBEDTLS_DEPRECATED_REMOVED)
#error "MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT is deprecated and will be removed in a future version of Mbed TLS"
#elif defined(MBEDTLS_DEPRECATED_WARNING)
#warning "MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT is deprecated and will be removed in a future version of Mbed TLS"
#endif
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT && MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT != 0 */

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC) &&   \
    !defined(MBEDTLS_SSL_PROTO_TLS1_2)
#error "MBEDTLS_SSL_ENCRYPT_THEN_MAC defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET) && \
    !defined(MBEDTLS_SSL_PROTO_TLS1_2)
#error "MBEDTLS_SSL_EXTENDED_MASTER_SECRET defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_RENEGOTIATION) && \
    !defined(MBEDTLS_SSL_PROTO_TLS1_2)
#error "MBEDTLS_SSL_RENEGOTIATION defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TICKET_C) && ( !defined(MBEDTLS_CIPHER_C) && \
                                       !defined(MBEDTLS_USE_PSA_CRYPTO) )
#error "MBEDTLS_SSL_TICKET_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TICKET_C) && \
    !( defined(MBEDTLS_SSL_HAVE_CCM) || defined(MBEDTLS_SSL_HAVE_GCM) || \
    defined(MBEDTLS_SSL_HAVE_CHACHAPOLY) )
#error "MBEDTLS_SSL_TICKET_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH) && \
    MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH >= 256
#error "MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH must be less than 256"
#endif

#if defined(MBEDTLS_SSL_SERVER_NAME_INDICATION) && \
        !defined(MBEDTLS_X509_CRT_PARSE_C)
#error "MBEDTLS_SSL_SERVER_NAME_INDICATION defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_THREADING_PTHREAD)
#if !defined(MBEDTLS_THREADING_C) || defined(MBEDTLS_THREADING_IMPL)
#error "MBEDTLS_THREADING_PTHREAD defined, but not all prerequisites"
#endif
#define MBEDTLS_THREADING_IMPL // undef at the end of this paragraph
#endif
#if defined(MBEDTLS_THREADING_ALT)
#if !defined(MBEDTLS_THREADING_C) || defined(MBEDTLS_THREADING_IMPL)
#error "MBEDTLS_THREADING_ALT defined, but not all prerequisites"
#endif
#define MBEDTLS_THREADING_IMPL // undef at the end of this paragraph
#endif
#if defined(MBEDTLS_THREADING_C) && !defined(MBEDTLS_THREADING_IMPL)
#error "MBEDTLS_THREADING_C defined, single threading implementation required"
#endif
#undef MBEDTLS_THREADING_IMPL // temporary macro defined above

#if defined(MBEDTLS_USE_PSA_CRYPTO) && !defined(MBEDTLS_PSA_CRYPTO_CLIENT)
#error "MBEDTLS_USE_PSA_CRYPTO defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_VERSION_FEATURES) && !defined(MBEDTLS_VERSION_C)
#error "MBEDTLS_VERSION_FEATURES defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_USE_C) && \
    (!defined(MBEDTLS_OID_C) || !defined(MBEDTLS_ASN1_PARSE_C) ||   \
    !defined(MBEDTLS_PK_PARSE_C) ||                                 \
    ( !defined(MBEDTLS_MD_C) && !defined(MBEDTLS_USE_PSA_CRYPTO) ) )
#error "MBEDTLS_X509_USE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CREATE_C) && \
    (!defined(MBEDTLS_OID_C) || !defined(MBEDTLS_ASN1_WRITE_C) ||      \
    !defined(MBEDTLS_PK_PARSE_C) ||                                    \
    ( !defined(MBEDTLS_MD_C) && !defined(MBEDTLS_USE_PSA_CRYPTO) ) )
#error "MBEDTLS_X509_CREATE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C) && ( !defined(MBEDTLS_X509_USE_C) )
#error "MBEDTLS_X509_CRT_PARSE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CRL_PARSE_C) && ( !defined(MBEDTLS_X509_USE_C) )
#error "MBEDTLS_X509_CRL_PARSE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CSR_PARSE_C) && ( !defined(MBEDTLS_X509_USE_C) )
#error "MBEDTLS_X509_CSR_PARSE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CRT_WRITE_C) && ( !defined(MBEDTLS_X509_CREATE_C) )
#error "MBEDTLS_X509_CRT_WRITE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_CSR_WRITE_C) && ( !defined(MBEDTLS_X509_CREATE_C) )
#error "MBEDTLS_X509_CSR_WRITE_C defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK) && \
            ( !defined(MBEDTLS_X509_CRT_PARSE_C) )
#error "MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_HAVE_INT32) && defined(MBEDTLS_HAVE_INT64)
#error "MBEDTLS_HAVE_INT32 and MBEDTLS_HAVE_INT64 cannot be defined simultaneously"
#endif /* MBEDTLS_HAVE_INT32 && MBEDTLS_HAVE_INT64 */

#if ( defined(MBEDTLS_HAVE_INT32) || defined(MBEDTLS_HAVE_INT64) ) && \
    defined(MBEDTLS_HAVE_ASM)
#error "MBEDTLS_HAVE_INT32/MBEDTLS_HAVE_INT64 and MBEDTLS_HAVE_ASM cannot be defined simultaneously"
#endif /* (MBEDTLS_HAVE_INT32 || MBEDTLS_HAVE_INT64) && MBEDTLS_HAVE_ASM */

#if defined(MBEDTLS_SSL_DTLS_SRTP) && ( !defined(MBEDTLS_SSL_PROTO_DTLS) )
#error "MBEDTLS_SSL_DTLS_SRTP defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH) && ( !defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH) )
#error "MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_RECORD_SIZE_LIMIT) && ( !defined(MBEDTLS_SSL_PROTO_TLS1_3) )
#error "MBEDTLS_SSL_RECORD_SIZE_LIMIT defined, but not all prerequisites"
#endif

#if defined(MBEDTLS_SSL_CONTEXT_SERIALIZATION) && \
    !( defined(MBEDTLS_SSL_HAVE_CCM) || defined(MBEDTLS_SSL_HAVE_GCM) || \
    defined(MBEDTLS_SSL_HAVE_CHACHAPOLY) )
#error "MBEDTLS_SSL_CONTEXT_SERIALIZATION defined, but not all prerequisites"
#endif

/* Reject attempts to enable options that have been removed and that could
 * cause a build to succeed but with features removed. */

#if defined(MBEDTLS_HAVEGE_C) //no-check-names
#error "MBEDTLS_HAVEGE_C was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/2599"
#endif

#if defined(MBEDTLS_SSL_HW_RECORD_ACCEL) //no-check-names
#error "MBEDTLS_SSL_HW_RECORD_ACCEL was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4031"
#endif

#if defined(MBEDTLS_SSL_PROTO_SSL3) //no-check-names
#error "MBEDTLS_SSL_PROTO_SSL3 (SSL v3.0 support) was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4031"
#endif

#if defined(MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO) //no-check-names
#error "MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO (SSL v2 ClientHello support) was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4031"
#endif

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT) //no-check-names
#error "MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT (compatibility with the buggy implementation of truncated HMAC in Mbed TLS up to 2.7) was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4031"
#endif

#if defined(MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES) //no-check-names
#error "MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES was removed in Mbed TLS 3.0. See the ChangeLog entry if you really need SHA-1-signed certificates."
#endif

#if defined(MBEDTLS_ZLIB_SUPPORT) //no-check-names
#error "MBEDTLS_ZLIB_SUPPORT was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4031"
#endif

#if defined(MBEDTLS_CHECK_PARAMS) //no-check-names
#error "MBEDTLS_CHECK_PARAMS was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4313"
#endif

#if defined(MBEDTLS_SSL_CID_PADDING_GRANULARITY) //no-check-names
#error "MBEDTLS_SSL_CID_PADDING_GRANULARITY was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4335"
#endif

#if defined(MBEDTLS_SSL_TLS1_3_PADDING_GRANULARITY) //no-check-names
#error "MBEDTLS_SSL_TLS1_3_PADDING_GRANULARITY was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4335"
#endif

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC) //no-check-names
#error "MBEDTLS_SSL_TRUNCATED_HMAC was removed in Mbed TLS 3.0. See https://github.com/Mbed-TLS/mbedtls/issues/4341"
#endif

#if defined(MBEDTLS_PKCS7_C) && ( ( !defined(MBEDTLS_ASN1_PARSE_C) ) || \
    ( !defined(MBEDTLS_OID_C) ) || ( !defined(MBEDTLS_PK_PARSE_C) ) || \
    ( !defined(MBEDTLS_X509_CRT_PARSE_C) ) || \
    ( !defined(MBEDTLS_X509_CRL_PARSE_C) ) || \
    ( !defined(MBEDTLS_MD_C) ) )
#error  "MBEDTLS_PKCS7_C is defined, but not all prerequisites"
#endif

/*
 * Avoid warning from -pedantic. This is a convenient place for this
 * workaround since this is included by every single file before the
 * #if defined(MBEDTLS_xxx_C) that results in empty translation units.
 */
typedef int mbedtls_iso_c_forbids_empty_translation_units;

/* *INDENT-ON* */
#endif /* MBEDTLS_CHECK_CONFIG_H */


#endif /* MBEDTLS_BUILD_INFO_H */

/* ===== Retained alignment.h ===== */

/**
 * \file alignment.h
 *
 * \brief Utility code for dealing with unaligned memory accesses
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/alignment.h.
 */

#ifndef MBEDTLS_LIBRARY_ALIGNMENT_H
#define MBEDTLS_LIBRARY_ALIGNMENT_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#if !defined(MBEDTLS_ALIGNMENT_DISABLE_EFFICENT_UNALIGNED_ACCESS)  //no-check-names
/*
 * Define MBEDTLS_EFFICIENT_UNALIGNED_ACCESS for architectures where unaligned memory
 * accesses are known to be efficient.
 *
 * All functions defined here will behave correctly regardless, but might be less
 * efficient when this is not defined.
 */
#if defined(__ARM_FEATURE_UNALIGNED) \
    || defined(MBEDTLS_ARCH_IS_X86) || defined(MBEDTLS_ARCH_IS_X64) \
    || defined(MBEDTLS_PLATFORM_IS_WINDOWS_ON_ARM64)
/*
 * __ARM_FEATURE_UNALIGNED is defined where appropriate by armcc, gcc 7, clang 9
 * (and later versions) for Arm v7 and later; all x86 platforms should have
 * efficient unaligned access.
 *
 * https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions?view=msvc-170#alignment
 * specifies that on Windows-on-Arm64, unaligned access is safe (except for uncached
 * device memory).
 */
#define MBEDTLS_EFFICIENT_UNALIGNED_ACCESS
#endif /* __ARM_FEATURE_UNALIGNED || MBEDTLS_ARCH_IS_X86 || MBEDTLS_ARCH_IS_X64 ||
        * MBEDTLS_PLATFORM_IS_WINDOWS_ON_ARM64 */
#endif /* MBEDTLS_ALIGNMENT_DISABLE_EFFICENT_UNALIGNED_ACCESS */ //no-check-names

#if defined(__IAR_SYSTEMS_ICC__) && \
    (defined(MBEDTLS_ARCH_IS_ARM64) || defined(MBEDTLS_ARCH_IS_ARM32) \
    || defined(__ICCRX__) || defined(__ICCRL78__) || defined(__ICCRISCV__))
#pragma language=save
#pragma language=extended
#define MBEDTLS_POP_IAR_LANGUAGE_PRAGMA
/* IAR recommend this technique for accessing unaligned data in
 * https://mypages.iar.com/s/article/Accessing-Unaligned-Data
 * This results in a single load / store instruction (if unaligned access is supported).
 * According to that document, this is only supported on certain architectures.
 */
#define UINT_UNALIGNED

/* Some products, like Zephyr, defines __packed as a macro for attribute(packed) and
 * that does not work with typedefs, so if __packed is defined, undef it for the
 * typedefs and restore it afterwards.
 */
#ifdef __packed
#pragma push_macro("__packed")
#undef __packed
#define MBEDTLS_IAR_PACKED_MACRO_USED
#endif

typedef uint16_t __packed mbedtls_uint16_unaligned_t;
typedef uint32_t __packed mbedtls_uint32_unaligned_t;
typedef uint64_t __packed mbedtls_uint64_unaligned_t;

#ifdef MBEDTLS_IAR_PACKED_MACRO_USED
#undef MBEDTLS_IAR_PACKED_MACRO_USED
#pragma pop_macro("__packed")
#endif

#elif defined(MBEDTLS_COMPILER_IS_GCC) && (MBEDTLS_GCC_VERSION >= 40504) && \
    ((MBEDTLS_GCC_VERSION < 60300) || (!defined(MBEDTLS_EFFICIENT_UNALIGNED_ACCESS)))
/*
 * gcc may generate a branch to memcpy for calls like `memcpy(dest, src, 4)` rather than
 * generating some LDR or LDRB instructions (similar for stores).
 *
 * This is architecture dependent: x86-64 seems fine even with old gcc; 32-bit Arm
 * is affected. To keep it simple, we enable for all architectures.
 *
 * For versions of gcc < 5.4.0 this issue always happens.
 * For gcc < 6.3.0, this issue happens at -O0
 * For all versions, this issue happens iff unaligned access is not supported.
 *
 * For gcc 4.x, this implementation will generate byte-by-byte loads even if unaligned access is
 * supported, which is correct but not optimal.
 *
 * For performance (and code size, in some cases), we want to avoid the branch and just generate
 * some inline load/store instructions since the access is small and constant-size.
 *
 * The manual states:
 * "The packed attribute specifies that a variable or structure field should have the smallest
 *  possible alignment—one byte for a variable"
 * https://gcc.gnu.org/onlinedocs/gcc-4.5.4/gcc/Variable-Attributes.html
 *
 * Previous implementations used __attribute__((__aligned__(1)), but had issues with a gcc bug:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94662
 *
 * Tested with several versions of GCC from 4.5.0 up to 13.2.0
 * We don't enable for older than 4.5.0 as this has not been tested.
 */
 #define UINT_UNALIGNED_STRUCT
typedef struct {
    uint16_t x;
} __attribute__((packed, may_alias)) mbedtls_uint16_unaligned_t;
typedef struct {
    uint32_t x;
} __attribute__((packed, may_alias)) mbedtls_uint32_unaligned_t;
typedef struct {
    uint64_t x;
} __attribute__((packed, may_alias)) mbedtls_uint64_unaligned_t;
 #endif

/*
 * We try to force mbedtls_(get|put)_unaligned_uintXX to be always inline, because this results
 * in code that is both smaller and faster. IAR and gcc both benefit from this when optimising
 * for size.
 */

/**
 * Read the unsigned 16 bits integer from the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 2 bytes of data
 * \return  Data at the given address
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline uint16_t mbedtls_get_unaligned_uint16(const void *p)
{
    uint16_t r;
#if defined(UINT_UNALIGNED)
    mbedtls_uint16_unaligned_t *p16 = (mbedtls_uint16_unaligned_t *) p;
    r = *p16;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint16_unaligned_t *p16 = (mbedtls_uint16_unaligned_t *) p;
    r = p16->x;
#else
    memcpy(&r, p, sizeof(r));
#endif
    return r;
}

/**
 * Write the unsigned 16 bits integer to the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 2 bytes of data
 * \param   x data to write
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline void mbedtls_put_unaligned_uint16(void *p, uint16_t x)
{
#if defined(UINT_UNALIGNED)
    mbedtls_uint16_unaligned_t *p16 = (mbedtls_uint16_unaligned_t *) p;
    *p16 = x;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint16_unaligned_t *p16 = (mbedtls_uint16_unaligned_t *) p;
    p16->x = x;
#else
    memcpy(p, &x, sizeof(x));
#endif
}

/**
 * Read the unsigned 32 bits integer from the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 4 bytes of data
 * \return  Data at the given address
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline uint32_t mbedtls_get_unaligned_uint32(const void *p)
{
    uint32_t r;
#if defined(UINT_UNALIGNED)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    r = *p32;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    r = p32->x;
#else
    memcpy(&r, p, sizeof(r));
#endif
    return r;
}

/**
 * Write the unsigned 32 bits integer to the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 4 bytes of data
 * \param   x data to write
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline void mbedtls_put_unaligned_uint32(void *p, uint32_t x)
{
#if defined(UINT_UNALIGNED)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    *p32 = x;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    p32->x = x;
#else
    memcpy(p, &x, sizeof(x));
#endif
}

/**
 * Read the unsigned 64 bits integer from the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 8 bytes of data
 * \return  Data at the given address
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline uint64_t mbedtls_get_unaligned_uint64(const void *p)
{
    uint64_t r;
#if defined(UINT_UNALIGNED)
    mbedtls_uint64_unaligned_t *p64 = (mbedtls_uint64_unaligned_t *) p;
    r = *p64;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint64_unaligned_t *p64 = (mbedtls_uint64_unaligned_t *) p;
    r = p64->x;
#else
    memcpy(&r, p, sizeof(r));
#endif
    return r;
}

/**
 * Write the unsigned 64 bits integer to the given address, which need not
 * be aligned.
 *
 * \param   p pointer to 8 bytes of data
 * \param   x data to write
 */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline void mbedtls_put_unaligned_uint64(void *p, uint64_t x)
{
#if defined(UINT_UNALIGNED)
    mbedtls_uint64_unaligned_t *p64 = (mbedtls_uint64_unaligned_t *) p;
    *p64 = x;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint64_unaligned_t *p64 = (mbedtls_uint64_unaligned_t *) p;
    p64->x = x;
#else
    memcpy(p, &x, sizeof(x));
#endif
}

#if defined(MBEDTLS_POP_IAR_LANGUAGE_PRAGMA)
#pragma language=restore
#endif

/** Byte Reading Macros
 *
 * Given a multi-byte integer \p x, MBEDTLS_BYTE_n retrieves the n-th
 * byte from x, where byte 0 is the least significant byte.
 */
#define MBEDTLS_BYTE_0(x) ((uint8_t) ((x)         & 0xff))
#define MBEDTLS_BYTE_1(x) ((uint8_t) (((x) >>  8) & 0xff))
#define MBEDTLS_BYTE_2(x) ((uint8_t) (((x) >> 16) & 0xff))
#define MBEDTLS_BYTE_3(x) ((uint8_t) (((x) >> 24) & 0xff))
#define MBEDTLS_BYTE_4(x) ((uint8_t) (((x) >> 32) & 0xff))
#define MBEDTLS_BYTE_5(x) ((uint8_t) (((x) >> 40) & 0xff))
#define MBEDTLS_BYTE_6(x) ((uint8_t) (((x) >> 48) & 0xff))
#define MBEDTLS_BYTE_7(x) ((uint8_t) (((x) >> 56) & 0xff))

/*
 * Detect GCC built-in byteswap routines
 */
#if defined(__GNUC__)
#if MBEDTLS_GCC_VERSION >= 40800
#define MBEDTLS_BSWAP16 __builtin_bswap16
#endif
#if MBEDTLS_GCC_VERSION >= 40300
#define MBEDTLS_BSWAP32 __builtin_bswap32
#define MBEDTLS_BSWAP64 __builtin_bswap64
#endif
#endif /* defined(__GNUC__) */

/*
 * Detect Clang built-in byteswap routines
 */
#if defined(__clang__) && defined(__has_builtin)
#if __has_builtin(__builtin_bswap16) && !defined(MBEDTLS_BSWAP16)
#define MBEDTLS_BSWAP16 __builtin_bswap16
#endif /* __has_builtin(__builtin_bswap16) */
#if __has_builtin(__builtin_bswap32) && !defined(MBEDTLS_BSWAP32)
#define MBEDTLS_BSWAP32 __builtin_bswap32
#endif /* __has_builtin(__builtin_bswap32) */
#if __has_builtin(__builtin_bswap64) && !defined(MBEDTLS_BSWAP64)
#define MBEDTLS_BSWAP64 __builtin_bswap64
#endif /* __has_builtin(__builtin_bswap64) */
#endif /* defined(__clang__) && defined(__has_builtin) */

/*
 * Detect MSVC built-in byteswap routines
 */
#if defined(_MSC_VER)
#if !defined(MBEDTLS_BSWAP16)
#define MBEDTLS_BSWAP16 _byteswap_ushort
#endif
#if !defined(MBEDTLS_BSWAP32)
#define MBEDTLS_BSWAP32 _byteswap_ulong
#endif
#if !defined(MBEDTLS_BSWAP64)
#define MBEDTLS_BSWAP64 _byteswap_uint64
#endif
#endif /* defined(_MSC_VER) */

/* Detect armcc built-in byteswap routine */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 410000) && !defined(MBEDTLS_BSWAP32)
#if defined(__ARM_ACLE)  /* ARM Compiler 6 - earlier versions don't need a header */
#include <arm_acle.h>
#endif
#define MBEDTLS_BSWAP32 __rev
#endif

/* Detect IAR built-in byteswap routine */
#if defined(__IAR_SYSTEMS_ICC__)
#if defined(__ARM_ACLE)
#include <arm_acle.h>
#define MBEDTLS_BSWAP16(x) ((uint16_t) __rev16((uint32_t) (x)))
#define MBEDTLS_BSWAP32 __rev
#define MBEDTLS_BSWAP64 __revll
#endif
#endif

/*
 * Where compiler built-ins are not present, fall back to C code that the
 * compiler may be able to detect and transform into the relevant bswap or
 * similar instruction.
 */
#if !defined(MBEDTLS_BSWAP16)
static inline uint16_t mbedtls_bswap16(uint16_t x)
{
    return
        (x & 0x00ff) << 8 |
        (x & 0xff00) >> 8;
}
#define MBEDTLS_BSWAP16 mbedtls_bswap16
#endif /* !defined(MBEDTLS_BSWAP16) */

#if !defined(MBEDTLS_BSWAP32)
static inline uint32_t mbedtls_bswap32(uint32_t x)
{
    return
        (x & 0x000000ff) << 24 |
        (x & 0x0000ff00) <<  8 |
        (x & 0x00ff0000) >>  8 |
        (x & 0xff000000) >> 24;
}
#define MBEDTLS_BSWAP32 mbedtls_bswap32
#endif /* !defined(MBEDTLS_BSWAP32) */

#if !defined(MBEDTLS_BSWAP64)
static inline uint64_t mbedtls_bswap64(uint64_t x)
{
    return
        (x & 0x00000000000000ffULL) << 56 |
        (x & 0x000000000000ff00ULL) << 40 |
        (x & 0x0000000000ff0000ULL) << 24 |
        (x & 0x00000000ff000000ULL) <<  8 |
        (x & 0x000000ff00000000ULL) >>  8 |
        (x & 0x0000ff0000000000ULL) >> 24 |
        (x & 0x00ff000000000000ULL) >> 40 |
        (x & 0xff00000000000000ULL) >> 56;
}
#define MBEDTLS_BSWAP64 mbedtls_bswap64
#endif /* !defined(MBEDTLS_BSWAP64) */

#if !defined(__BYTE_ORDER__)

#if defined(__LITTLE_ENDIAN__)
/* IAR defines __xxx_ENDIAN__, but not __BYTE_ORDER__ */
#define MBEDTLS_IS_BIG_ENDIAN 0
#elif defined(__BIG_ENDIAN__)
#define MBEDTLS_IS_BIG_ENDIAN 1
#else
static const uint16_t mbedtls_byte_order_detector = { 0x100 };
#define MBEDTLS_IS_BIG_ENDIAN (*((unsigned char *) (&mbedtls_byte_order_detector)) == 0x01)
#endif

#else

#if (__BYTE_ORDER__) == (__ORDER_BIG_ENDIAN__)
#define MBEDTLS_IS_BIG_ENDIAN 1
#else
#define MBEDTLS_IS_BIG_ENDIAN 0
#endif

#endif /* !defined(__BYTE_ORDER__) */

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT32_BE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? mbedtls_get_unaligned_uint32((data) + (offset))                  \
        : MBEDTLS_BSWAP32(mbedtls_get_unaligned_uint32((data) + (offset))) \
    )

/**
 * Put in memory a 32 bits unsigned integer in big-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT32_BE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), (uint32_t) (n));     \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), MBEDTLS_BSWAP32((uint32_t) (n))); \
        }                                                                        \
    }

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT32_LE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? MBEDTLS_BSWAP32(mbedtls_get_unaligned_uint32((data) + (offset))) \
        : mbedtls_get_unaligned_uint32((data) + (offset))                  \
    )

/**
 * Put in memory a 32 bits unsigned integer in little-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT32_LE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), MBEDTLS_BSWAP32((uint32_t) (n))); \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), ((uint32_t) (n)));   \
        }                                                                        \
    }

/**
 * Get the unsigned 16 bits integer corresponding to two bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the two bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the two bytes to build the 16 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT16_LE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? MBEDTLS_BSWAP16(mbedtls_get_unaligned_uint16((data) + (offset))) \
        : mbedtls_get_unaligned_uint16((data) + (offset))                  \
    )

/**
 * Put in memory a 16 bits unsigned integer in little-endian order.
 *
 * \param   n       16 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 16
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 16 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT16_LE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint16((data) + (offset), MBEDTLS_BSWAP16((uint16_t) (n))); \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint16((data) + (offset), (uint16_t) (n));     \
        }                                                                        \
    }

/**
 * Get the unsigned 16 bits integer corresponding to two bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the two bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the two bytes to build the 16 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT16_BE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? mbedtls_get_unaligned_uint16((data) + (offset))                  \
        : MBEDTLS_BSWAP16(mbedtls_get_unaligned_uint16((data) + (offset))) \
    )

/**
 * Put in memory a 16 bits unsigned integer in big-endian order.
 *
 * \param   n       16 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 16
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 16 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT16_BE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint16((data) + (offset), (uint16_t) (n));     \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint16((data) + (offset), MBEDTLS_BSWAP16((uint16_t) (n))); \
        }                                                                        \
    }

/**
 * Get the unsigned 24 bits integer corresponding to three bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the three bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the three bytes to build the 24 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT24_BE(data, offset)        \
    (                                              \
        ((uint32_t) (data)[(offset)] << 16)        \
        | ((uint32_t) (data)[(offset) + 1] << 8)   \
        | ((uint32_t) (data)[(offset) + 2])        \
    )

/**
 * Put in memory a 24 bits unsigned integer in big-endian order.
 *
 * \param   n       24 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 24
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 24 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT24_BE(n, data, offset)                \
    {                                                         \
        (data)[(offset)] = MBEDTLS_BYTE_2(n);                 \
        (data)[(offset) + 1] = MBEDTLS_BYTE_1(n);             \
        (data)[(offset) + 2] = MBEDTLS_BYTE_0(n);             \
    }

/**
 * Get the unsigned 24 bits integer corresponding to three bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the three bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the three bytes to build the 24 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT24_LE(data, offset)               \
    (                                                     \
        ((uint32_t) (data)[(offset)])                     \
        | ((uint32_t) (data)[(offset) + 1] <<  8)         \
        | ((uint32_t) (data)[(offset) + 2] << 16)         \
    )

/**
 * Put in memory a 24 bits unsigned integer in little-endian order.
 *
 * \param   n       24 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 24
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 24 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT24_LE(n, data, offset)                \
    {                                                         \
        (data)[(offset)] = MBEDTLS_BYTE_0(n);                 \
        (data)[(offset) + 1] = MBEDTLS_BYTE_1(n);             \
        (data)[(offset) + 2] = MBEDTLS_BYTE_2(n);             \
    }

/**
 * Get the unsigned 64 bits integer corresponding to eight bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the eight bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the eight bytes to build the 64 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT64_BE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? mbedtls_get_unaligned_uint64((data) + (offset))                  \
        : MBEDTLS_BSWAP64(mbedtls_get_unaligned_uint64((data) + (offset))) \
    )

/**
 * Put in memory a 64 bits unsigned integer in big-endian order.
 *
 * \param   n       64 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 64
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 64 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT64_BE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint64((data) + (offset), (uint64_t) (n));     \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint64((data) + (offset), MBEDTLS_BSWAP64((uint64_t) (n))); \
        }                                                                        \
    }

/**
 * Get the unsigned 64 bits integer corresponding to eight bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the eight bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the eight bytes to build the 64 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT64_LE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? MBEDTLS_BSWAP64(mbedtls_get_unaligned_uint64((data) + (offset))) \
        : mbedtls_get_unaligned_uint64((data) + (offset))                  \
    )

/**
 * Put in memory a 64 bits unsigned integer in little-endian order.
 *
 * \param   n       64 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 64
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 64 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT64_LE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint64((data) + (offset), MBEDTLS_BSWAP64((uint64_t) (n))); \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint64((data) + (offset), (uint64_t) (n));     \
        }                                                                        \
    }

#endif /* MBEDTLS_LIBRARY_ALIGNMENT_H */


#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#define MBEDTLS_HAVE_NEON_INTRINSICS
#elif defined(MBEDTLS_PLATFORM_IS_WINDOWS_ON_ARM64)
#include <arm64_neon.h>
#define MBEDTLS_HAVE_NEON_INTRINSICS
#endif

/* Decide whether we're built for a Unix-like platform.
 */
#if defined(MBEDTLS_TEST_PLATFORM_IS_NOT_UNIXLIKE) //no-check-names
/* We may be building on a Unix-like platform, but for test purposes,
 * do not try to use Unix features. */
#elif defined(_WIN32)
/* If Windows platform interfaces are available, we use them, even if
 * a Unix-like might also to be available. */
/* defined(_WIN32) ==> we can include <windows.h> */
#elif defined(unix) || defined(__unix) || defined(__unix__) ||    \
    (defined(__APPLE__) && defined(__MACH__)) ||                  \
    defined(__HAIKU__) ||                                         \
    defined(__midipix__) ||                                       \
    /* Add other Unix-like platform indicators here ^^^^ */ 0
/* defined(MBEDTLS_PLATFORM_IS_UNIXLIKE) ==> we can include <unistd.h> */
#define MBEDTLS_PLATFORM_IS_UNIXLIKE
#endif

/** Helper to define a function as static except when building invasive tests.
 *
 * If a function is only used inside its own source file and should be
 * declared `static` to allow the compiler to optimize for code size,
 * but that function has unit tests, define it with
 * ```
 * MBEDTLS_STATIC_TESTABLE int mbedtls_foo(...) { ... }
 * ```
 * and declare it in a header in the `library/` directory with
 * ```
 * #if defined(MBEDTLS_TEST_HOOKS)
 * int mbedtls_foo(...);
 * #endif
 * ```
 */
#define MBEDTLS_STATIC_TESTABLE

#define MBEDTLS_TEST_HOOK_TEST_ASSERT(TEST)

/** \def ARRAY_LENGTH
 * Return the number of elements of a static or stack array.
 *
 * \param array         A value of array (not pointer) type.
 *
 * \return The number of elements of the array.
 */
/* A correct implementation of ARRAY_LENGTH, but which silently gives
 * a nonsensical result if called with a pointer rather than an array. */
#define ARRAY_LENGTH_UNSAFE(array)            \
    (sizeof(array) / sizeof(*(array)))

#if defined(__GNUC__)
/* Test if arg and &(arg)[0] have the same type. This is true if arg is
 * an array but not if it's a pointer. */
#define IS_ARRAY_NOT_POINTER(arg)                                     \
    (!__builtin_types_compatible_p(__typeof__(arg),                \
                                   __typeof__(&(arg)[0])))
/* A compile-time constant with the value 0. If `const_expr` is not a
 * compile-time constant with a nonzero value, cause a compile-time error. */
#define STATIC_ASSERT_EXPR(const_expr)                                \
    (0 && sizeof(struct { unsigned int STATIC_ASSERT : 1 - 2 * !(const_expr); }))

/* Return the scalar value `value` (possibly promoted). This is a compile-time
 * constant if `value` is. `condition` must be a compile-time constant.
 * If `condition` is false, arrange to cause a compile-time error. */
#define STATIC_ASSERT_THEN_RETURN(condition, value)   \
    (STATIC_ASSERT_EXPR(condition) ? 0 : (value))

#define ARRAY_LENGTH(array)                                           \
    (STATIC_ASSERT_THEN_RETURN(IS_ARRAY_NOT_POINTER(array),         \
                               ARRAY_LENGTH_UNSAFE(array)))

#else
/* If we aren't sure the compiler supports our non-standard tricks,
 * fall back to the unsafe implementation. */
#define ARRAY_LENGTH(array) ARRAY_LENGTH_UNSAFE(array)
#endif

#if defined(__has_builtin)
#define MBEDTLS_HAS_BUILTIN(x) __has_builtin(x)
#else
#define MBEDTLS_HAS_BUILTIN(x) 0
#endif

/** Allow library to access its structs' private members.
 *
 * Although structs defined in header files are publicly available,
 * their members are private and should not be accessed by the user.
 */
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

/**
 * \brief       Securely zeroize a buffer then free it.
 *
 *              Similar to making consecutive calls to
 *              \c mbedtls_platform_zeroize() and \c mbedtls_free(), but has
 *              code size savings, and potential for optimisation in the future.
 *
 *              Guaranteed to be a no-op if \p buf is \c NULL and \p len is 0.
 *
 * \param buf   Buffer to be zeroized then freed.
 * \param len   Length of the buffer in bytes
 */
void mbedtls_zeroize_and_free(void *buf, size_t len);

/* Always inline mbedtls_xor() for similar reasons as mbedtls_xor_no_simd(). */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
/**
 * Perform a fast block XOR operation, such that
 * r[i] = a[i] ^ b[i] where 0 <= i < n
 *
 * \param   r Pointer to result (buffer of at least \p n bytes). \p r
 *            may be equal to either \p a or \p b, but behaviour when
 *            it overlaps in other ways is undefined.
 * \param   a Pointer to input (buffer of at least \p n bytes)
 * \param   b Pointer to input (buffer of at least \p n bytes)
 * \param   n Number of bytes to process.
 *
 * \note      Depending on the situation, it may be faster to use either mbedtls_xor() or
 *            mbedtls_xor_no_simd() (these are functionally equivalent).
 *            If the result is used immediately after the xor operation in non-SIMD code (e.g, in
 *            AES-CBC), there may be additional latency to transfer the data from SIMD to scalar
 *            registers, and in this case, mbedtls_xor_no_simd() may be faster. In other cases where
 *            the result is not used immediately (e.g., in AES-CTR), mbedtls_xor() may be faster.
 *            For targets without SIMD support, they will behave the same.
 */
static inline void mbedtls_xor(unsigned char *r,
                               const unsigned char *a,
                               const unsigned char *b,
                               size_t n)
{
    size_t i = 0;
#if defined(MBEDTLS_EFFICIENT_UNALIGNED_ACCESS)
#if defined(MBEDTLS_HAVE_NEON_INTRINSICS) && \
    (!(defined(MBEDTLS_COMPILER_IS_GCC) && MBEDTLS_GCC_VERSION < 70300))
    /* Old GCC versions generate a warning here, so disable the NEON path for these compilers */
    for (; (i + 16) <= n; i += 16) {
        uint8x16_t v1 = vld1q_u8(a + i);
        uint8x16_t v2 = vld1q_u8(b + i);
        uint8x16_t x = veorq_u8(v1, v2);
        vst1q_u8(r + i, x);
    }
#if defined(__IAR_SYSTEMS_ICC__)
    /* This if statement helps some compilers (e.g., IAR) optimise out the byte-by-byte tail case
     * where n is a constant multiple of 16.
     * For other compilers (e.g. recent gcc and clang) it makes no difference if n is a compile-time
     * constant, and is a very small perf regression if n is not a compile-time constant. */
    if (n % 16 == 0) {
        return;
    }
#endif
#if defined(MBEDTLS_COMPILER_IS_GCC) && MBEDTLS_HAS_BUILTIN(__builtin_constant_p)
    if (__builtin_constant_p(n) && n % 16 == 0) {
        return;
    }
#endif
#elif defined(MBEDTLS_ARCH_IS_X64) || defined(MBEDTLS_ARCH_IS_ARM64)
    /* This codepath probably only makes sense on architectures with 64-bit registers */
    for (; (i + 8) <= n; i += 8) {
        uint64_t x = mbedtls_get_unaligned_uint64(a + i) ^ mbedtls_get_unaligned_uint64(b + i);
        mbedtls_put_unaligned_uint64(r + i, x);
    }
#if defined(__IAR_SYSTEMS_ICC__)
    if (n % 8 == 0) {
        return;
    }
#endif
#if defined(MBEDTLS_COMPILER_IS_GCC) && MBEDTLS_HAS_BUILTIN(__builtin_constant_p)
    if (__builtin_constant_p(n) && n % 8 == 0) {
        return;
    }
#endif
#else
    for (; (i + 4) <= n; i += 4) {
        uint32_t x = mbedtls_get_unaligned_uint32(a + i) ^ mbedtls_get_unaligned_uint32(b + i);
        mbedtls_put_unaligned_uint32(r + i, x);
    }
#if defined(__IAR_SYSTEMS_ICC__)
    if (n % 4 == 0) {
        return;
    }
#endif
#if defined(MBEDTLS_COMPILER_IS_GCC) && MBEDTLS_HAS_BUILTIN(__builtin_constant_p)
    if (__builtin_constant_p(n) && n % 4 == 0) {
        return;
    }
#endif
#endif
#endif
    for (; i < n; i++) {
        r[i] = a[i] ^ b[i];
    }
}

/* Always inline mbedtls_xor_no_simd() as we see significant perf regressions when it does not get
 * inlined (e.g., observed about 3x perf difference in gcm_mult_largetable with gcc 7 - 12) */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma inline = forced
#elif defined(__GNUC__)
__attribute__((always_inline))
#endif
/**
 * Perform a fast block XOR operation, such that
 * r[i] = a[i] ^ b[i] where 0 <= i < n
 *
 * In some situations, this can perform better than mbedtls_xor() (e.g., it's about 5%
 * better in AES-CBC).
 *
 * \param   r Pointer to result (buffer of at least \p n bytes). \p r
 *            may be equal to either \p a or \p b, but behaviour when
 *            it overlaps in other ways is undefined.
 * \param   a Pointer to input (buffer of at least \p n bytes)
 * \param   b Pointer to input (buffer of at least \p n bytes)
 * \param   n Number of bytes to process.
 *
 * \note      Depending on the situation, it may be faster to use either mbedtls_xor() or
 *            mbedtls_xor_no_simd() (these are functionally equivalent).
 *            If the result is used immediately after the xor operation in non-SIMD code (e.g, in
 *            AES-CBC), there may be additional latency to transfer the data from SIMD to scalar
 *            registers, and in this case, mbedtls_xor_no_simd() may be faster. In other cases where
 *            the result is not used immediately (e.g., in AES-CTR), mbedtls_xor() may be faster.
 *            For targets without SIMD support, they will behave the same.
 */
static inline void mbedtls_xor_no_simd(unsigned char *r,
                                       const unsigned char *a,
                                       const unsigned char *b,
                                       size_t n)
{
    size_t i = 0;
#if defined(MBEDTLS_EFFICIENT_UNALIGNED_ACCESS)
#if defined(MBEDTLS_ARCH_IS_X64) || defined(MBEDTLS_ARCH_IS_ARM64)
    /* This codepath probably only makes sense on architectures with 64-bit registers */
    for (; (i + 8) <= n; i += 8) {
        uint64_t x = mbedtls_get_unaligned_uint64(a + i) ^ mbedtls_get_unaligned_uint64(b + i);
        mbedtls_put_unaligned_uint64(r + i, x);
    }
#if defined(__IAR_SYSTEMS_ICC__)
    /* This if statement helps some compilers (e.g., IAR) optimise out the byte-by-byte tail case
     * where n is a constant multiple of 8.
     * For other compilers (e.g. recent gcc and clang) it makes no difference if n is a compile-time
     * constant, and is a very small perf regression if n is not a compile-time constant. */
    if (n % 8 == 0) {
        return;
    }
#endif
#else
    for (; (i + 4) <= n; i += 4) {
        uint32_t x = mbedtls_get_unaligned_uint32(a + i) ^ mbedtls_get_unaligned_uint32(b + i);
        mbedtls_put_unaligned_uint32(r + i, x);
    }
#if defined(__IAR_SYSTEMS_ICC__)
    if (n % 4 == 0) {
        return;
    }
#endif
#endif
#endif
    for (; i < n; i++) {
        r[i] = a[i] ^ b[i];
    }
}

/* Fix MSVC C99 compatible issue
 *      MSVC support __func__ from visual studio 2015( 1900 )
 *      Use MSVC predefine macro to avoid name check fail.
 */
#if (defined(_MSC_VER) && (_MSC_VER <= 1900))
#define /*no-check-names*/ __func__ __FUNCTION__
#endif

/* Define `asm` for compilers which don't define it. */
/* *INDENT-OFF* */
#ifndef asm
#if defined(__IAR_SYSTEMS_ICC__)
#define asm __asm
#else
#define asm __asm__
#endif
#endif
/* *INDENT-ON* */

/*
 * Define the constraint used for read-only pointer operands to aarch64 asm.
 *
 * This is normally the usual "r", but for aarch64_32 (aka ILP32,
 * as found in watchos), "p" is required to avoid warnings from clang.
 *
 * Note that clang does not recognise '+p' or '=p', and armclang
 * does not recognise 'p' at all. Therefore, to update a pointer from
 * aarch64 assembly, it is necessary to use something like:
 *
 * uintptr_t uptr = (uintptr_t) ptr;
 * asm( "ldr x4, [%x0], #8" ... : "+r" (uptr) : : )
 * ptr = (void*) uptr;
 *
 * Note that the "x" in "%x0" is neccessary; writing "%0" will cause warnings.
 */
#if defined(__aarch64__) && defined(MBEDTLS_HAVE_ASM)
#if UINTPTR_MAX == 0xfffffffful
/* ILP32: Specify the pointer operand slightly differently, as per #7787. */
#define MBEDTLS_ASM_AARCH64_PTR_CONSTRAINT "p"
#elif UINTPTR_MAX == 0xfffffffffffffffful
/* Normal case (64-bit pointers): use "r" as the constraint for pointer operands to asm */
#define MBEDTLS_ASM_AARCH64_PTR_CONSTRAINT "r"
#else
#error "Unrecognised pointer size for aarch64"
#endif
#endif

/* Always provide a static assert macro, so it can be used unconditionally.
 * It does nothing on systems where we don't know how to define a static assert.
 */
/* Can't use the C11-style `defined(static_assert)` on FreeBSD, since it
 * defines static_assert even with -std=c99, but then complains about it.
 */
#if defined(static_assert) && !defined(__FreeBSD__)
#define MBEDTLS_STATIC_ASSERT(expr, msg)    static_assert(expr, msg)
#else
/* Make sure `MBEDTLS_STATIC_ASSERT(expr, msg);` is valid both inside and
 * outside a function. We choose a struct declaration, which can be repeated
 * any number of times and does not need a matching definition. */
#define MBEDTLS_STATIC_ASSERT(expr, msg)                                \
    struct ISO_C_does_not_allow_extra_semicolon_outside_of_a_function
#endif

/* Define compiler branch hints */
#if MBEDTLS_HAS_BUILTIN(__builtin_expect)
#define MBEDTLS_LIKELY(x)       __builtin_expect(!!(x), 1)
#define MBEDTLS_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define MBEDTLS_LIKELY(x)       x
#define MBEDTLS_UNLIKELY(x)     x
#endif

/* MBEDTLS_ASSUME may be used to provide additional information to the compiler
 * which can result in smaller code-size. */
#if MBEDTLS_HAS_BUILTIN(__builtin_assume)
/* clang provides __builtin_assume */
#define MBEDTLS_ASSUME(x)       __builtin_assume(x)
#elif MBEDTLS_HAS_BUILTIN(__builtin_unreachable)
/* gcc and IAR can use __builtin_unreachable */
#define MBEDTLS_ASSUME(x)       do { if (!(x)) __builtin_unreachable(); } while (0)
#elif defined(_MSC_VER)
/* Supported by MSVC since VS 2005 */
#define MBEDTLS_ASSUME(x)       __assume(x)
#else
#define MBEDTLS_ASSUME(x)       do { } while (0)
#endif

/* For gcc -Os, override with -O2 for a given function.
 *
 * This will not affect behaviour for other optimisation settings, e.g. -O0.
 */
#if defined(MBEDTLS_COMPILER_IS_GCC) && defined(__OPTIMIZE_SIZE__)
#define MBEDTLS_OPTIMIZE_FOR_PERFORMANCE __attribute__((optimize("-O2")))
#else
#define MBEDTLS_OPTIMIZE_FOR_PERFORMANCE
#endif

/* Suppress compiler warnings for unused functions and variables. */
#if !defined(MBEDTLS_MAYBE_UNUSED) && defined(__has_attribute)
#    if __has_attribute(unused)
#        define MBEDTLS_MAYBE_UNUSED __attribute__((unused))
#    endif
#endif
#if !defined(MBEDTLS_MAYBE_UNUSED) && defined(__GNUC__)
#    define MBEDTLS_MAYBE_UNUSED __attribute__((unused))
#endif
#if !defined(MBEDTLS_MAYBE_UNUSED) && defined(__IAR_SYSTEMS_ICC__) && defined(__VER__)
/* IAR does support __attribute__((unused)), but only if the -e flag (extended language support)
 * is given; the pragma always works.
 * Unfortunately the pragma affects the rest of the file where it is used, but this is harmless.
 * Check for version 5.2 or later - this pragma may be supported by earlier versions, but I wasn't
 * able to find documentation).
 */
#    if (__VER__ >= 5020000)
#        define MBEDTLS_MAYBE_UNUSED _Pragma("diag_suppress=Pe177")
#    endif
#endif
#if !defined(MBEDTLS_MAYBE_UNUSED) && defined(_MSC_VER)
#    define MBEDTLS_MAYBE_UNUSED __pragma(warning(suppress:4189))
#endif
#if !defined(MBEDTLS_MAYBE_UNUSED)
#    define MBEDTLS_MAYBE_UNUSED
#endif

/* GCC >= 15 has a warning 'unterminated-string-initialization' which complains if you initialize
 * a string into an array without space for a terminating NULL character. In some places in the
 * codebase this behaviour is intended, so we add the macro MBEDTLS_ATTRIBUTE_UNTERMINATED_STRING
 * to suppress the warning in these places.
 */
#if defined(__has_attribute)
#if __has_attribute(nonstring)
#define MBEDTLS_HAS_ATTRIBUTE_NONSTRING
#endif /* __has_attribute(nonstring) */
#endif /* __has_attribute */
#if defined(MBEDTLS_HAS_ATTRIBUTE_NONSTRING)
#define MBEDTLS_ATTRIBUTE_UNTERMINATED_STRING __attribute__((nonstring))
#else
#define MBEDTLS_ATTRIBUTE_UNTERMINATED_STRING
#endif /* MBEDTLS_HAS_ATTRIBUTE_NONSTRING */

#endif /* MBEDTLS_LIBRARY_COMMON_H */

/* ===== Retained ecdh.h ===== */

/**
 * \file ecdh.h
 *
 * \brief This file contains ECDH definitions and functions.
 *
 * The Elliptic Curve Diffie-Hellman (ECDH) protocol is an anonymous
 * key agreement protocol allowing two parties to establish a shared
 * secret over an insecure channel. Each party must have an
 * elliptic-curve public private key pair.
 *
 * For more information, see <em>NIST SP 800-56A Rev. 2: Recommendation for
 * Pair-Wise Key Establishment Schemes Using Discrete Logarithm
 * Cryptography</em>.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ecdh.h.
 */

#ifndef MBEDTLS_ECDH_H
#define MBEDTLS_ECDH_H
/* ===== Retained private_access.h ===== */

/**
 * \file private_access.h
 *
 * \brief Macro wrapper for struct's members.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/private_access.h.
 */

#ifndef MBEDTLS_PRIVATE_ACCESS_H
#define MBEDTLS_PRIVATE_ACCESS_H

#ifndef MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_PRIVATE(member) private_##member
#else
#define MBEDTLS_PRIVATE(member) member
#endif

#endif /* MBEDTLS_PRIVATE_ACCESS_H */



/* ===== Retained ecp.h ===== */

/**
 * \file ecp.h
 *
 * \brief This file provides an API for Elliptic Curves over GF(P) (ECP).
 *
 * The use of ECP in cryptography and TLS is defined in
 * <em>Standards for Efficient Cryptography Group (SECG): SEC1
 * Elliptic Curve Cryptography</em> and
 * <em>RFC-4492: Elliptic Curve Cryptography (ECC) Cipher Suites
 * for Transport Layer Security (TLS)</em>.
 *
 * <em>RFC-2409: The Internet Key Exchange (IKE)</em> defines ECP
 * group types.
 *
 */

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ecp.h.
 */

#ifndef MBEDTLS_ECP_H
#define MBEDTLS_ECP_H

/* ===== Retained platform_util.h ===== */

/**
 * \file platform_util.h
 *
 * \brief Common and shared functions used by multiple modules in the Mbed TLS
 *        library.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/platform_util.h.
 */
#ifndef MBEDTLS_PLATFORM_UTIL_H
#define MBEDTLS_PLATFORM_UTIL_H


#include <stddef.h>
/* ===== Retained platform_time.h ===== */

/**
 * \file platform_time.h
 *
 * \brief Mbed TLS Platform time abstraction
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/platform_time.h.
 */
#ifndef MBEDTLS_PLATFORM_TIME_H
#define MBEDTLS_PLATFORM_TIME_H


#ifdef __cplusplus
extern "C" {
#endif

/*
 * The time_t datatype
 */
#if defined(MBEDTLS_PLATFORM_TIME_TYPE_MACRO)
typedef MBEDTLS_PLATFORM_TIME_TYPE_MACRO mbedtls_time_t;
#else
/* For time_t */
#include <time.h>
typedef time_t mbedtls_time_t;
#endif /* MBEDTLS_PLATFORM_TIME_TYPE_MACRO */

#if defined(MBEDTLS_PLATFORM_MS_TIME_TYPE_MACRO)
typedef MBEDTLS_PLATFORM_MS_TIME_TYPE_MACRO mbedtls_ms_time_t;
#else
#include <stdint.h>
#include <inttypes.h>
typedef int64_t mbedtls_ms_time_t;
#endif

/*
 * The function pointers for time
 */
#if defined(MBEDTLS_PLATFORM_TIME_ALT)
extern mbedtls_time_t (*mbedtls_time)(mbedtls_time_t *time);

/**
 * \brief   Set your own time function pointer
 *
 * \param   time_func   the time function implementation
 *
 * \return              0
 */
int mbedtls_platform_set_time(mbedtls_time_t (*time_func)(mbedtls_time_t *time));
#else
#if defined(MBEDTLS_PLATFORM_TIME_MACRO)
#define mbedtls_time    MBEDTLS_PLATFORM_TIME_MACRO
#else
#define mbedtls_time   time
#endif /* MBEDTLS_PLATFORM_TIME_MACRO */
#endif /* MBEDTLS_PLATFORM_TIME_ALT */

#ifdef __cplusplus
}
#endif

#endif /* platform_time.h */

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal helper macros for deprecating API constants. */
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED __attribute__((deprecated))
MBEDTLS_DEPRECATED typedef char const *mbedtls_deprecated_string_constant_t;
#define MBEDTLS_DEPRECATED_STRING_CONSTANT(VAL)       \
    ((mbedtls_deprecated_string_constant_t) (VAL))
MBEDTLS_DEPRECATED typedef int mbedtls_deprecated_numeric_constant_t;
#define MBEDTLS_DEPRECATED_NUMERIC_CONSTANT(VAL)       \
    ((mbedtls_deprecated_numeric_constant_t) (VAL))
#else
#define MBEDTLS_DEPRECATED
#define MBEDTLS_DEPRECATED_STRING_CONSTANT(VAL) VAL
#define MBEDTLS_DEPRECATED_NUMERIC_CONSTANT(VAL) VAL
#endif /* MBEDTLS_DEPRECATED_WARNING */
#endif /* MBEDTLS_DEPRECATED_REMOVED */

/* Implementation of the check-return facility.
 * See the user documentation in mbedtls_config.h.
 *
 * Do not use this macro directly to annotate function: instead,
 * use one of MBEDTLS_CHECK_RETURN_CRITICAL or MBEDTLS_CHECK_RETURN_TYPICAL
 * depending on how important it is to check the return value.
 */
#if !defined(MBEDTLS_CHECK_RETURN)
#if defined(__GNUC__)
#define MBEDTLS_CHECK_RETURN __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER) && _MSC_VER >= 1700
#include <sal.h>
#define MBEDTLS_CHECK_RETURN _Check_return_
#else
#define MBEDTLS_CHECK_RETURN
#endif
#endif

/** Critical-failure function
 *
 * This macro appearing at the beginning of the declaration of a function
 * indicates that its return value should be checked in all applications.
 * Omitting the check is very likely to indicate a bug in the application
 * and will result in a compile-time warning if #MBEDTLS_CHECK_RETURN
 * is implemented for the compiler in use.
 *
 * \note  The use of this macro is a work in progress.
 *        This macro may be added to more functions in the future.
 *        Such an extension is not considered an API break, provided that
 *        there are near-unavoidable circumstances under which the function
 *        can fail. For example, signature/MAC/AEAD verification functions,
 *        and functions that require a random generator, are considered
 *        return-check-critical.
 */
#define MBEDTLS_CHECK_RETURN_CRITICAL MBEDTLS_CHECK_RETURN

/** Ordinary-failure function
 *
 * This macro appearing at the beginning of the declaration of a function
 * indicates that its return value should be generally be checked in portable
 * applications. Omitting the check will result in a compile-time warning if
 * #MBEDTLS_CHECK_RETURN is implemented for the compiler in use and
 * #MBEDTLS_CHECK_RETURN_WARNING is enabled in the compile-time configuration.
 *
 * You can use #MBEDTLS_IGNORE_RETURN to explicitly ignore the return value
 * of a function that is annotated with #MBEDTLS_CHECK_RETURN.
 *
 * \note  The use of this macro is a work in progress.
 *        This macro will be added to more functions in the future.
 *        Eventually this should appear before most functions returning
 *        an error code (as \c int in the \c mbedtls_xxx API or
 *        as ::psa_status_t in the \c psa_xxx API).
 */
#if defined(MBEDTLS_CHECK_RETURN_WARNING)
#define MBEDTLS_CHECK_RETURN_TYPICAL MBEDTLS_CHECK_RETURN
#else
#define MBEDTLS_CHECK_RETURN_TYPICAL
#endif

/** Benign-failure function
 *
 * This macro appearing at the beginning of the declaration of a function
 * indicates that it is rarely useful to check its return value.
 *
 * This macro has an empty expansion. It exists for documentation purposes:
 * a #MBEDTLS_CHECK_RETURN_OPTIONAL annotation indicates that the function
 * has been analyzed for return-check usefulness, whereas the lack of
 * an annotation indicates that the function has not been analyzed and its
 * return-check usefulness is unknown.
 */
#define MBEDTLS_CHECK_RETURN_OPTIONAL

/** \def MBEDTLS_IGNORE_RETURN
 *
 * Call this macro with one argument, a function call, to suppress a warning
 * from #MBEDTLS_CHECK_RETURN due to that function call.
 */
#if !defined(MBEDTLS_IGNORE_RETURN)
/* GCC doesn't silence the warning with just (void)(result).
 * (void)!(result) is known to work up at least up to GCC 10, as well
 * as with Clang and MSVC.
 *
 * https://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Non_002dbugs.html
 * https://stackoverflow.com/questions/40576003/ignoring-warning-wunused-result
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425#c34
 */
#define MBEDTLS_IGNORE_RETURN(result) ((void) !(result))
#endif

/* If the following macro is defined, the library is being built by the test
 * framework, and the framework is going to provide a replacement
 * mbedtls_platform_zeroize() using a preprocessor macro, so the function
 * declaration should be omitted.  */
#if !defined(MBEDTLS_TEST_DEFINES_ZEROIZE) //no-check-names
/**
 * \brief       Securely zeroize a buffer
 *
 *              The function is meant to wipe the data contained in a buffer so
 *              that it can no longer be recovered even if the program memory
 *              is later compromised. Call this function on sensitive data
 *              stored on the stack before returning from a function, and on
 *              sensitive data stored on the heap before freeing the heap
 *              object.
 *
 *              It is extremely difficult to guarantee that calls to
 *              mbedtls_platform_zeroize() are not removed by aggressive
 *              compiler optimizations in a portable way. For this reason, Mbed
 *              TLS provides the configuration option
 *              MBEDTLS_PLATFORM_ZEROIZE_ALT, which allows users to configure
 *              mbedtls_platform_zeroize() to use a suitable implementation for
 *              their platform and needs
 *
 * \param buf   Buffer to be zeroized
 * \param len   Length of the buffer in bytes
 *
 */
void mbedtls_platform_zeroize(void *buf, size_t len);
#endif

/** \brief              The type of custom random generator (RNG) callbacks.
 *
 *                      Many Mbed TLS functions take two parameters
 *                      `mbedtls_f_rng_t *f_rng, void *p_rng`. The
 *                      library will call \c f_rng to generate
 *                      random values.
 *
 * \note                This is typically one of the following:
 *                      - mbedtls_ctr_drbg_random() with \c p_rng
 *                        pointing to a #mbedtls_ctr_drbg_context;
 *                      - mbedtls_hmac_drbg_random() with \c p_rng
 *                        pointing to a #mbedtls_hmac_drbg_context;
 *                      - mbedtls_psa_get_random() with
 *                        `prng = MBEDTLS_PSA_RANDOM_STATE`.
 *
 * \note                Generally, given a call
 *                      `mbedtls_foo(f_rng, p_rng, ....)`, the RNG callback
 *                      and the context only need to remain valid until
 *                      the call to `mbedtls_foo` returns. However, there
 *                      are a few exceptions where the callback is stored
 *                      in for future use. Check the documentation of
 *                      the calling function.
 *
 * \warning             In a multithreaded environment, calling the
 *                      function should be thread-safe. The standard
 *                      functions provided by the library are thread-safe
 *                      when #MBEDTLS_THREADING_C is enabled.
 *
 * \warning             This function must either provide as many
 *                      bytes as requested of **cryptographic quality**
 *                      random data, or return a negative error code.
 *
 * \param p_rng         The \c p_rng argument that was passed along \c f_rng.
 *                      The library always passes \c p_rng unchanged.
 *                      This is typically a pointer to the random generator
 *                      state, or \c NULL if the custom random generator
 *                      doesn't need a context-specific state.
 * \param[out] output   On success, this must be filled with \p output_size
 *                      bytes of cryptographic-quality random data.
 * \param output_size   The number of bytes to output.
 *
 * \return              \c 0 on success, or a negative error code on failure.
 *                      Library functions will generally propagate this
 *                      error code, so \c MBEDTLS_ERR_xxx values are
 *                      recommended. #MBEDTLS_ERR_ENTROPY_SOURCE_FAILED is
 *                      typically sensible for RNG failures.
 */
typedef int mbedtls_f_rng_t(void *p_rng,
                            unsigned char *output, size_t output_size, MinTlsDiagnostics* diagnostics);

/**
 * \brief      Platform-specific implementation of gmtime_r()
 *
 *             The function is a thread-safe abstraction that behaves
 *             similarly to the gmtime_r() function from Unix/POSIX.
 *
 *             Mbed TLS will try to identify the underlying platform and
 *             make use of an appropriate underlying implementation (e.g.
 *             gmtime_r() for POSIX and gmtime_s() for Windows). If this is
 *             not possible, then gmtime() will be used. In this case, calls
 *             from the library to gmtime() will be guarded by the mutex
 *             mbedtls_threading_gmtime_mutex if MBEDTLS_THREADING_C is
 *             enabled. It is recommended that calls from outside the library
 *             are also guarded by this mutex.
 *
 *             If MBEDTLS_PLATFORM_GMTIME_R_ALT is defined, then Mbed TLS will
 *             unconditionally use the alternative implementation for
 *             mbedtls_platform_gmtime_r() supplied by the user at compile time.
 *
 * \param tt     Pointer to an object containing time (in seconds) since the
 *               epoch to be converted
 * \param tm_buf Pointer to an object where the results will be stored
 *
 * \return      Pointer to an object of type struct tm on success, otherwise
 *              NULL
 */
struct tm *mbedtls_platform_gmtime_r(const mbedtls_time_t *tt,
                                     struct tm *tm_buf);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_PLATFORM_UTIL_H */


/* ===== Retained bignum.h ===== */

/**
 * \file bignum.h
 *
 * \brief Multi-precision integer library
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/bignum.h.
 */
#ifndef MBEDTLS_BIGNUM_H
#define MBEDTLS_BIGNUM_H


#include <stddef.h>
#include <stdint.h>

/** An error occurred while reading from or writing to a file. */
#define MBEDTLS_ERR_MPI_FILE_IO_ERROR                     -0x0002
/** Bad input parameters to function. */
#define MBEDTLS_ERR_MPI_BAD_INPUT_DATA                    -0x0004
/** There is an invalid character in the digit string. */
#define MBEDTLS_ERR_MPI_INVALID_CHARACTER                 -0x0006
/** The buffer is too small to write to. */
#define MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL                  -0x0008
/** The input arguments are negative or result in illegal output. */
#define MBEDTLS_ERR_MPI_NEGATIVE_VALUE                    -0x000A
/** The input argument for division is zero, which is not allowed. */
#define MBEDTLS_ERR_MPI_DIVISION_BY_ZERO                  -0x000C
/** The input arguments are not acceptable. */
#define MBEDTLS_ERR_MPI_NOT_ACCEPTABLE                    -0x000E
/** Memory allocation failed. */
#define MBEDTLS_ERR_MPI_ALLOC_FAILED                      -0x0010

#define MBEDTLS_MPI_CHK(f)       \
    do                           \
    {                            \
        if ((ret = (f)) != 0) \
        goto cleanup;        \
    } while (0)

/*
 * Maximum size MPIs are allowed to grow to in number of limbs.
 */
#define MBEDTLS_MPI_MAX_LIMBS                             10000

#if !defined(MBEDTLS_MPI_WINDOW_SIZE)
/*
 * Maximum window size used for modular exponentiation. Default: 3
 * Minimum value: 1. Maximum value: 6.
 *
 * Result is an array of ( 2 ** MBEDTLS_MPI_WINDOW_SIZE ) MPIs used
 * for the sliding window calculation. (So 8 by default)
 *
 * Reduction in size, reduces speed.
 */
#define MBEDTLS_MPI_WINDOW_SIZE                           3        /**< Maximum window size used. */
#endif /* !MBEDTLS_MPI_WINDOW_SIZE */

#define MBEDTLS_MPI_MAX_BITS                              (8 * MBEDTLS_MPI_MAX_SIZE)      /**< Maximum number of bits for usable MPIs. */

/*
 * When reading from files with mbedtls_mpi_read_file() and writing to files with
 * mbedtls_mpi_write_file() the buffer should have space
 * for a (short) label, the MPI (in the provided radix), the newline
 * characters and the '\0'.
 *
 * By default we assume at least a 10 char label, a minimum radix of 10
 * (decimal) and a maximum of 4096 bit numbers (1234 decimal chars).
 * Autosized at compile time for at least a 10 char label, a minimum radix
 * of 10 (decimal) for a number of MBEDTLS_MPI_MAX_BITS size.
 *
 * This used to be statically sized to 1250 for a maximum of 4096 bit
 * numbers (1234 decimal chars).
 *
 * Calculate using the formula:
 *  MBEDTLS_MPI_RW_BUFFER_SIZE = ceil(MBEDTLS_MPI_MAX_BITS / ln(10) * ln(2)) +
 *                                LabelSize + 6
 */
#define MBEDTLS_MPI_MAX_BITS_SCALE100          (100 * MBEDTLS_MPI_MAX_BITS)
#define MBEDTLS_LN_2_DIV_LN_10_SCALE100                 332
#define MBEDTLS_MPI_RW_BUFFER_SIZE             (((MBEDTLS_MPI_MAX_BITS_SCALE100 + \
                                                  MBEDTLS_LN_2_DIV_LN_10_SCALE100 - 1) / \
                                                 MBEDTLS_LN_2_DIV_LN_10_SCALE100) + 10 + 6)

/*
 * Define the base integer type, architecture-wise.
 *
 * 32 or 64-bit integer types can be forced regardless of the underlying
 * architecture by defining MBEDTLS_HAVE_INT32 or MBEDTLS_HAVE_INT64
 * respectively and undefining MBEDTLS_HAVE_ASM.
 *
 * Double-width integers (e.g. 128-bit in 64-bit architectures) can be
 * disabled by defining MBEDTLS_NO_UDBL_DIVISION.
 */
#if !defined(MBEDTLS_HAVE_INT32)
    #if defined(_MSC_VER) && defined(_M_AMD64)
/* Always choose 64-bit when using MSC */
        #if !defined(MBEDTLS_HAVE_INT64)
            #define MBEDTLS_HAVE_INT64
        #endif /* !MBEDTLS_HAVE_INT64 */
typedef  int64_t mbedtls_mpi_sint;
typedef uint64_t mbedtls_mpi_uint;
#define MBEDTLS_MPI_UINT_MAX                UINT64_MAX
    #elif defined(__GNUC__) && (                         \
    defined(__amd64__) || defined(__x86_64__)     || \
    defined(__ppc64__) || defined(__powerpc64__)  || \
    defined(__ia64__)  || defined(__alpha__)      || \
    (defined(__sparc__) && defined(__arch64__)) || \
    defined(__s390x__) || defined(__mips64)       || \
    defined(__aarch64__))
        #if !defined(MBEDTLS_HAVE_INT64)
            #define MBEDTLS_HAVE_INT64
        #endif /* MBEDTLS_HAVE_INT64 */
typedef  int64_t mbedtls_mpi_sint;
typedef uint64_t mbedtls_mpi_uint;
#define MBEDTLS_MPI_UINT_MAX                UINT64_MAX
        #if !defined(MBEDTLS_NO_UDBL_DIVISION)
/* mbedtls_t_udbl defined as 128-bit unsigned int */
typedef unsigned int mbedtls_t_udbl __attribute__((mode(TI)));
            #define MBEDTLS_HAVE_UDBL
        #endif /* !MBEDTLS_NO_UDBL_DIVISION */
    #elif defined(__ARMCC_VERSION) && defined(__aarch64__)
/*
 * __ARMCC_VERSION is defined for both armcc and armclang and
 * __aarch64__ is only defined by armclang when compiling 64-bit code
 */
        #if !defined(MBEDTLS_HAVE_INT64)
            #define MBEDTLS_HAVE_INT64
        #endif /* !MBEDTLS_HAVE_INT64 */
typedef  int64_t mbedtls_mpi_sint;
typedef uint64_t mbedtls_mpi_uint;
#define MBEDTLS_MPI_UINT_MAX                UINT64_MAX
        #if !defined(MBEDTLS_NO_UDBL_DIVISION)
/* mbedtls_t_udbl defined as 128-bit unsigned int */
typedef __uint128_t mbedtls_t_udbl;
            #define MBEDTLS_HAVE_UDBL
        #endif /* !MBEDTLS_NO_UDBL_DIVISION */
    #elif defined(MBEDTLS_HAVE_INT64)
/* Force 64-bit integers with unknown compiler */
typedef  int64_t mbedtls_mpi_sint;
typedef uint64_t mbedtls_mpi_uint;
#define MBEDTLS_MPI_UINT_MAX                UINT64_MAX
    #endif
#endif /* !MBEDTLS_HAVE_INT32 */

#if !defined(MBEDTLS_HAVE_INT64)
/* Default to 32-bit compilation */
    #if !defined(MBEDTLS_HAVE_INT32)
        #define MBEDTLS_HAVE_INT32
    #endif /* !MBEDTLS_HAVE_INT32 */
typedef  int32_t mbedtls_mpi_sint;
typedef uint32_t mbedtls_mpi_uint;
#define MBEDTLS_MPI_UINT_MAX                UINT32_MAX
    #if !defined(MBEDTLS_NO_UDBL_DIVISION)
typedef uint64_t mbedtls_t_udbl;
        #define MBEDTLS_HAVE_UDBL
    #endif /* !MBEDTLS_NO_UDBL_DIVISION */
#endif /* !MBEDTLS_HAVE_INT64 */

/*
 * Sanity check that exactly one of MBEDTLS_HAVE_INT32 or MBEDTLS_HAVE_INT64 is defined,
 * so that code elsewhere doesn't have to check.
 */
#if (!(defined(MBEDTLS_HAVE_INT32) || defined(MBEDTLS_HAVE_INT64))) || \
    (defined(MBEDTLS_HAVE_INT32) && defined(MBEDTLS_HAVE_INT64))
#error "Only 32-bit or 64-bit limbs are supported in bignum"
#endif

/** \typedef mbedtls_mpi_uint
 * \brief The type of machine digits in a bignum, called _limbs_.
 *
 * This is always an unsigned integer type with no padding bits. The size
 * is platform-dependent.
 */

/** \typedef mbedtls_mpi_sint
 * \brief The signed type corresponding to #mbedtls_mpi_uint.
 *
 * This is always an signed integer type with no padding bits. The size
 * is platform-dependent.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          MPI structure
 */
typedef struct mbedtls_mpi {
    /** Pointer to limbs.
     *
     * This may be \c NULL if \c n is 0.
     */
    mbedtls_mpi_uint *MBEDTLS_PRIVATE(p);

    /** Sign: -1 if the mpi is negative, 1 otherwise.
     *
     * The number 0 must be represented with `s = +1`. Although many library
     * functions treat all-limbs-zero as equivalent to a valid representation
     * of 0 regardless of the sign bit, there are exceptions, so bignum
     * functions and external callers must always set \c s to +1 for the
     * number zero.
     *
     * Note that this implies that calloc() or `... = {0}` does not create
     * a valid MPI representation. You must call mbedtls_mpi_init().
     */
    signed short MBEDTLS_PRIVATE(s);

    /** Total number of limbs in \c p.  */
    unsigned short MBEDTLS_PRIVATE(n);
    /* Make sure that MBEDTLS_MPI_MAX_LIMBS fits in n.
     * Use the same limit value on all platforms so that we don't have to
     * think about different behavior on the rare platforms where
     * unsigned short can store values larger than the minimum required by
     * the C language, which is 65535.
     */
#if MBEDTLS_MPI_MAX_LIMBS > 65535
#error "MBEDTLS_MPI_MAX_LIMBS > 65535 is not supported"
#endif
}
mbedtls_mpi;

/**
 * \brief           Initialize an MPI context.
 *
 *                  This makes the MPI ready to be set or freed,
 *                  but does not define a value for the MPI.
 *
 * \param X         The MPI context to initialize. This must not be \c NULL.
 */
void mbedtls_mpi_init(mbedtls_mpi *X);

/**
 * \brief          This function frees the components of an MPI context.
 *
 * \param X        The MPI context to be cleared. This may be \c NULL,
 *                 in which case this function is a no-op. If it is
 *                 not \c NULL, it must point to an initialized MPI.
 */
void mbedtls_mpi_free(mbedtls_mpi *X);

/**
 * \brief          Enlarge an MPI to the specified number of limbs.
 *
 * \note           This function does nothing if the MPI is
 *                 already large enough.
 *
 * \param X        The MPI to grow. It must be initialized.
 * \param nblimbs  The target number of limbs.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_grow(mbedtls_mpi *X, size_t nblimbs, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function resizes an MPI downwards, keeping at least the
 *                 specified number of limbs.
 *
 *                 If \c X is smaller than \c nblimbs, it is resized up
 *                 instead.
 *
 * \param X        The MPI to shrink. This must point to an initialized MPI.
 * \param nblimbs  The minimum number of limbs to keep.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed
 *                 (this can only happen when resizing up).
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_shrink(mbedtls_mpi *X, size_t nblimbs, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Make a copy of an MPI.
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param Y        The source MPI. This must point to an initialized MPI.
 *
 * \note           The limb-buffer in the destination MPI is enlarged
 *                 if necessary to hold the value in the source MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_copy(mbedtls_mpi *X, const mbedtls_mpi *Y, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a safe conditional copy of MPI which doesn't
 *                 reveal whether the condition was true or not.
 *
 * \param X        The MPI to conditionally assign to. This must point
 *                 to an initialized MPI.
 * \param Y        The MPI to be assigned from. This must point to an
 *                 initialized MPI.
 * \param assign   The condition deciding whether to perform the
 *                 assignment or not. Must be either 0 or 1:
 *                 * \c 1: Perform the assignment `X = Y`.
 *                 * \c 0: Keep the original value of \p X.
 *
 * \note           This function is equivalent to
 *                      `if( assign ) mbedtls_mpi_copy( X, Y );`
 *                 except that it avoids leaking any information about whether
 *                 the assignment was done or not (the above code may leak
 *                 information through branch prediction and/or memory access
 *                 patterns analysis).
 *
 * \warning        If \p assign is neither 0 nor 1, the result of this function
 *                 is indeterminate, and the resulting value in \p X might be
 *                 neither its original value nor the value in \p Y.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_safe_cond_assign(mbedtls_mpi *X, const mbedtls_mpi *Y, unsigned char assign, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Store integer value in MPI.
 *
 * \param X        The MPI to set. This must be initialized.
 * \param z        The value to use.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_lset(mbedtls_mpi *X, mbedtls_mpi_sint z, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Get a specific bit from an MPI.
 *
 * \param X        The MPI to query. This must be initialized.
 * \param pos      Zero-based index of the bit to query.
 *
 * \return         \c 0 or \c 1 on success, depending on whether bit \c pos
 *                 of \c X is unset or set.
 * \return         A negative error code on failure.
 */
int mbedtls_mpi_get_bit(const mbedtls_mpi *X, size_t pos);

/**
 * \brief          Return the number of bits of value \c 0 before the
 *                 least significant bit of value \c 1.
 *
 * \note           This is the same as the zero-based index of
 *                 the least significant bit of value \c 1.
 *
 * \param X        The MPI to query.
 *
 * \return         The number of bits of value \c 0 before the least significant
 *                 bit of value \c 1 in \p X.
 */
size_t mbedtls_mpi_lsb(const mbedtls_mpi *X);

/**
 * \brief          Return the number of bits up to and including the most
 *                 significant bit of value \c 1.
 *
 * * \note         This is same as the one-based index of the most
 *                 significant bit of value \c 1.
 *
 * \param X        The MPI to query. This must point to an initialized MPI.
 *
 * \return         The number of bits up to and including the most
 *                 significant bit of value \c 1.
 */
size_t mbedtls_mpi_bitlen(const mbedtls_mpi *X);

/**
 * \brief          Return the total size of an MPI value in bytes.
 *
 * \param X        The MPI to use. This must point to an initialized MPI.
 *
 * \note           The value returned by this function may be less than
 *                 the number of bytes used to store \p X internally.
 *                 This happens if and only if there are trailing bytes
 *                 of value zero.
 *
 * \return         The least number of bytes capable of storing
 *                 the absolute value of \p X.
 */
size_t mbedtls_mpi_size(const mbedtls_mpi *X);

/**
 * \brief          Import an MPI from unsigned big endian binary data.
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param buf      The input buffer. This must be a readable buffer of length
 *                 \p buflen Bytes.
 * \param buflen   The length of the input buffer \p buf in Bytes.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_read_binary(mbedtls_mpi *X, const unsigned char *buf,
                            size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Export X into unsigned binary data, big endian.
 *                 Always fills the whole buffer, which will start with zeros
 *                 if the number is smaller.
 *
 * \param X        The source MPI. This must point to an initialized MPI.
 * \param buf      The output buffer. This must be a writable buffer of length
 *                 \p buflen Bytes.
 * \param buflen   The size of the output buffer \p buf in Bytes.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL if \p buf isn't
 *                 large enough to hold the value of \p X.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_write_binary(const mbedtls_mpi *X, unsigned char *buf,
                             size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Export X into unsigned binary data, little endian.
 *                 Always fills the whole buffer, which will end with zeros
 *                 if the number is smaller.
 *
 * \param X        The source MPI. This must point to an initialized MPI.
 * \param buf      The output buffer. This must be a writable buffer of length
 *                 \p buflen Bytes.
 * \param buflen   The size of the output buffer \p buf in Bytes.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL if \p buf isn't
 *                 large enough to hold the value of \p X.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_write_binary_le(const mbedtls_mpi *X,
                                unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a left-shift on an MPI: X <<= count
 *
 * \param X        The MPI to shift. This must point to an initialized MPI.
 *                 The MPI pointed by \p X may be resized to fit
 *                 the resulting number.
 * \param count    The number of bits to shift by.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_shift_l(mbedtls_mpi *X, size_t count, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a right-shift on an MPI: X >>= count
 *
 * \param X        The MPI to shift. This must point to an initialized MPI.
 * \param count    The number of bits to shift by.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_shift_r(mbedtls_mpi *X, size_t count);

/**
 * \brief          Compare the absolute values of two MPIs.
 *
 * \param X        The left-hand MPI. This must point to an initialized MPI.
 * \param Y        The right-hand MPI. This must point to an initialized MPI.
 *
 * \return         \c 1 if `|X|` is greater than `|Y|`.
 * \return         \c -1 if `|X|` is lesser than `|Y|`.
 * \return         \c 0 if `|X|` is equal to `|Y|`.
 */
int mbedtls_mpi_cmp_abs(const mbedtls_mpi *X, const mbedtls_mpi *Y);

/**
 * \brief          Compare two MPIs.
 *
 * \param X        The left-hand MPI. This must point to an initialized MPI.
 * \param Y        The right-hand MPI. This must point to an initialized MPI.
 *
 * \return         \c 1 if \p X is greater than \p Y.
 * \return         \c -1 if \p X is lesser than \p Y.
 * \return         \c 0 if \p X is equal to \p Y.
 */
int mbedtls_mpi_cmp_mpi(const mbedtls_mpi *X, const mbedtls_mpi *Y);

/**
 * \brief          Compare an MPI with an integer.
 *
 * \param X        The left-hand MPI. This must point to an initialized MPI.
 * \param z        The integer value to compare \p X to.
 *
 * \return         \c 1 if \p X is greater than \p z.
 * \return         \c -1 if \p X is lesser than \p z.
 * \return         \c 0 if \p X is equal to \p z.
 */
int mbedtls_mpi_cmp_int(const mbedtls_mpi *X, mbedtls_mpi_sint z);

/**
 * \brief          Perform an unsigned addition of MPIs: X = |A| + |B|
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The first summand. This must point to an initialized MPI.
 * \param B        The second summand. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_add_abs(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform an unsigned subtraction of MPIs: X = |A| - |B|
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The minuend. This must point to an initialized MPI.
 * \param B        The subtrahend. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_NEGATIVE_VALUE if \p B is greater than \p A.
 * \return         Another negative error code on different kinds of failure.
 *
 */
int mbedtls_mpi_sub_abs(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a signed addition of MPIs: X = A + B
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The first summand. This must point to an initialized MPI.
 * \param B        The second summand. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_add_mpi(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a signed subtraction of MPIs: X = A - B
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The minuend. This must point to an initialized MPI.
 * \param B        The subtrahend. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_sub_mpi(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a signed addition of an MPI and an integer: X = A + b
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The first summand. This must point to an initialized MPI.
 * \param b        The second summand.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_add_int(mbedtls_mpi *X, const mbedtls_mpi *A,
                        mbedtls_mpi_sint b, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a signed subtraction of an MPI and an integer:
 *                 X = A - b
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The minuend. This must point to an initialized MPI.
 * \param b        The subtrahend.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_sub_int(mbedtls_mpi *X, const mbedtls_mpi *A,
                        mbedtls_mpi_sint b, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a multiplication of two MPIs: X = A * B
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The first factor. This must point to an initialized MPI.
 * \param B        The second factor. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 *
 */
int mbedtls_mpi_mul_mpi(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a multiplication of an MPI with an unsigned integer:
 *                 X = A * b
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param A        The first factor. This must point to an initialized MPI.
 * \param b        The second factor.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 *
 */
int mbedtls_mpi_mul_int(mbedtls_mpi *X, const mbedtls_mpi *A,
                        mbedtls_mpi_uint b, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a division with remainder of two MPIs:
 *                 A = Q * B + R
 *
 * \param Q        The destination MPI for the quotient.
 *                 This may be \c NULL if the value of the
 *                 quotient is not needed. This must not alias A or B.
 * \param R        The destination MPI for the remainder value.
 *                 This may be \c NULL if the value of the
 *                 remainder is not needed. This must not alias A or B.
 * \param A        The dividend. This must point to an initialized MPI.
 * \param B        The divisor. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_DIVISION_BY_ZERO if \p B equals zero.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_div_mpi(mbedtls_mpi *Q, mbedtls_mpi *R, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a modular reduction. R = A mod B
 *
 * \param R        The destination MPI for the residue value.
 *                 This must point to an initialized MPI.
 * \param A        The MPI to compute the residue of.
 *                 This must point to an initialized MPI.
 * \param B        The base of the modular reduction.
 *                 This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_DIVISION_BY_ZERO if \p B equals zero.
 * \return         #MBEDTLS_ERR_MPI_NEGATIVE_VALUE if \p B is negative.
 * \return         Another negative error code on different kinds of failure.
 *
 */
int mbedtls_mpi_mod_mpi(mbedtls_mpi *R, const mbedtls_mpi *A,
                        const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform a modular exponentiation: X = A^E mod N
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 *                 This must not alias E or N.
 * \param A        The base of the exponentiation.
 *                 This must point to an initialized MPI.
 * \param E        The exponent MPI. This must point to an initialized MPI.
 * \param N        The base for the modular reduction. This must point to an
 *                 initialized MPI.
 * \param prec_RR  A helper MPI depending solely on \p N which can be used to
 *                 speed-up multiple modular exponentiations for the same value
 *                 of \p N. This may be \c NULL. If it is not \c NULL, it must
 *                 point to an initialized MPI. If it hasn't been used after
 *                 the call to mbedtls_mpi_init(), this function will compute
 *                 the helper value and store it in \p prec_RR for reuse on
 *                 subsequent calls to this function. Otherwise, the function
 *                 will assume that \p prec_RR holds the helper value set by a
 *                 previous call to mbedtls_mpi_exp_mod(), and reuse it.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if \c N is negative or
 *                 even, or if \c E is negative.
 * \return         Another negative error code on different kinds of failures.
 *
 */
int mbedtls_mpi_exp_mod(mbedtls_mpi *X, const mbedtls_mpi *A,
                        const mbedtls_mpi *E, const mbedtls_mpi *N,
                        mbedtls_mpi *prec_RR, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Fill an MPI with a number of random bytes.
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param size     The number of random bytes to generate.
 * \param f_rng    The RNG function to use. This must not be \c NULL.
 * \param p_rng    The RNG parameter to be passed to \p f_rng. This may be
 *                 \c NULL if \p f_rng doesn't need a context argument.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on failure.
 *
 * \note           The bytes obtained from the RNG are interpreted
 *                 as a big-endian representation of an MPI; this can
 *                 be relevant in applications like deterministic ECDSA.
 */
int mbedtls_mpi_fill_random(mbedtls_mpi *X, size_t size,
                            mbedtls_f_rng_t *f_rng,
                            void *p_rng, MinTlsDiagnostics* diagnostics);

/** Generate a random number uniformly in a range.
 *
 * This function generates a random number between \p min inclusive and
 * \p N exclusive.
 *
 * The procedure complies with RFC 6979 §3.3 (deterministic ECDSA)
 * when the RNG is a suitably parametrized instance of HMAC_DRBG
 * and \p min is \c 1.
 *
 * \note           There are `N - min` possible outputs. The lower bound
 *                 \p min can be reached, but the upper bound \p N cannot.
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param min      The minimum value to return.
 *                 It must be nonnegative.
 * \param N        The upper bound of the range, exclusive.
 *                 In other words, this is one plus the maximum value to return.
 *                 \p N must be strictly larger than \p min.
 * \param f_rng    The RNG function to use. This must not be \c NULL.
 * \param p_rng    The RNG parameter to be passed to \p f_rng.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if \p min or \p N is invalid
 *                 or if they are incompatible.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if the implementation was
 *                 unable to find a suitable value within a limited number
 *                 of attempts. This has a negligible probability if \p N
 *                 is significantly larger than \p min, which is the case
 *                 for all usual cryptographic applications.
 * \return         Another negative error code on failure.
 */
int mbedtls_mpi_random(mbedtls_mpi *X,
                       mbedtls_mpi_sint min,
                       const mbedtls_mpi *N,
                       mbedtls_f_rng_t *f_rng,
                       void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Compute the greatest common divisor: G = gcd(A, B)
 *
 * \param G        The destination MPI. This must point to an initialized MPI.
 *                 This will always be positive or 0.
 * \param A        The first operand. This must point to an initialized MPI.
 * \param B        The second operand. This must point to an initialized MPI.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         Another negative error code on different kinds of failure.
 */
int mbedtls_mpi_gcd(mbedtls_mpi *G, const mbedtls_mpi *A,
                    const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Miller-Rabin primality test.
 *
 * \warning        If \p X is potentially generated by an adversary, for example
 *                 when validating cryptographic parameters that you didn't
 *                 generate yourself and that are supposed to be prime, then
 *                 \p rounds should be at least the half of the security
 *                 strength of the cryptographic algorithm. On the other hand,
 *                 if \p X is chosen uniformly or non-adversarially (as is the
 *                 case when mbedtls_mpi_gen_prime calls this function), then
 *                 \p rounds can be much lower.
 *
 * \param X        The MPI to check for primality.
 *                 This must point to an initialized MPI.
 * \param rounds   The number of bases to perform the Miller-Rabin primality
 *                 test for. The probability of returning 0 on a composite is
 *                 at most 2<sup>-2*\p rounds </sup>.
 * \param f_rng    The RNG function to use. This must not be \c NULL.
 * \param p_rng    The RNG parameter to be passed to \p f_rng.
 *                 This may be \c NULL if \p f_rng doesn't use
 *                 a context parameter.
 *
 * \return         \c 0 if successful, i.e. \p X is probably prime.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if \p X is not prime.
 * \return         Another negative error code on other kinds of failure.
 */
int mbedtls_mpi_is_prime_ext(const mbedtls_mpi *X, int rounds,
                             mbedtls_f_rng_t *f_rng,
                             void *p_rng, MinTlsDiagnostics* diagnostics);
/**
 * \brief Flags for mbedtls_mpi_gen_prime()
 *
 * Each of these flags is a constraint on the result X returned by
 * mbedtls_mpi_gen_prime().
 */
typedef enum {
    MBEDTLS_MPI_GEN_PRIME_FLAG_DH =      0x0001, /**< (X-1)/2 is prime too */
    MBEDTLS_MPI_GEN_PRIME_FLAG_LOW_ERR = 0x0002, /**< lower error rate from 2<sup>-80</sup> to 2<sup>-128</sup> */
} mbedtls_mpi_gen_prime_flag_t;

#ifdef __cplusplus
}
#endif

#endif /* bignum.h */


/*
 * ECP error codes
 */
/** Bad input parameters to function. */
#define MBEDTLS_ERR_ECP_BAD_INPUT_DATA                    -0x4F80
/** The buffer is too small to write to. */
#define MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL                  -0x4F00
/** The requested feature is not available, for example, the requested curve is not supported. */
#define MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE               -0x4E80
/** The signature is not valid. */
#define MBEDTLS_ERR_ECP_VERIFY_FAILED                     -0x4E00
/** Memory allocation failed. */
#define MBEDTLS_ERR_ECP_ALLOC_FAILED                      -0x4D80
/** Generation of random value, such as ephemeral key, failed. */
#define MBEDTLS_ERR_ECP_RANDOM_FAILED                     -0x4D00
/** Invalid private or public key. */
#define MBEDTLS_ERR_ECP_INVALID_KEY                       -0x4C80
/** The buffer contains a valid signature followed by more data. */
#define MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH                  -0x4C00
/** Operation in progress, call again with the same parameters to continue. */
#define MBEDTLS_ERR_ECP_IN_PROGRESS                       -0x4B00

/* Flags indicating whether to include code that is specific to certain
 * types of curves. These flags are for internal library use only. */
#define MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Domain-parameter identifiers: curve, subgroup, and generator.
 *
 * \note Only curves over prime fields are supported.
 *
 * \warning This library does not support validation of arbitrary domain
 * parameters. Therefore, only standardized domain parameters from trusted
 * sources should be used. See mbedtls_ecp_group_load().
 */
/* Note: when adding a new curve:
 * - Add it at the end of this enum, otherwise you'll break the ABI by
 *   changing the numerical value for existing curves.
 * - Increment MBEDTLS_ECP_DP_MAX below if needed.
 * - Update the calculation of MBEDTLS_ECP_MAX_BITS below.
 * - Add the corresponding MBEDTLS_ECP_DP_xxx_ENABLED macro definition to
 *   mbedtls_config.h.
 * - List the curve as a dependency of MBEDTLS_ECP_C and
 *   MBEDTLS_ECDSA_C if supported in check_config.h.
 * - Add the curve to the appropriate curve type macro
 *   MBEDTLS_ECP_yyy_ENABLED above.
 * - Add the necessary definitions to ecp_curves.c.
 * - Add the curve to the ecp_supported_curves array in ecp.c.
 * - Add the curve to applicable profiles in x509_crt.c.
 * - Add the curve to applicable presets in ssl_tls.c.
 */
typedef enum {
    MBEDTLS_ECP_DP_NONE = 0,       /*!< Curve not defined. */
    MBEDTLS_ECP_DP_SECP192R1,      /*!< Domain parameters for the 192-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SECP224R1,      /*!< Domain parameters for the 224-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SECP256R1,      /*!< Domain parameters for the 256-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SECP384R1,      /*!< Domain parameters for the 384-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SECP521R1,      /*!< Domain parameters for the 521-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_BP256R1,        /*!< Domain parameters for 256-bit Brainpool curve. */
    MBEDTLS_ECP_DP_BP384R1,        /*!< Domain parameters for 384-bit Brainpool curve. */
    MBEDTLS_ECP_DP_BP512R1,        /*!< Domain parameters for 512-bit Brainpool curve. */
    MBEDTLS_ECP_DP_CURVE25519,     /*!< Domain parameters for Curve25519. */
    MBEDTLS_ECP_DP_SECP192K1,      /*!< Domain parameters for 192-bit "Koblitz" curve. */
    MBEDTLS_ECP_DP_SECP224K1,      /*!< Domain parameters for 224-bit "Koblitz" curve. */
    MBEDTLS_ECP_DP_SECP256K1,      /*!< Domain parameters for 256-bit "Koblitz" curve. */
    MBEDTLS_ECP_DP_CURVE448,       /*!< Domain parameters for Curve448. */
} mbedtls_ecp_group_id;

/**
 * The number of supported curves, plus one for #MBEDTLS_ECP_DP_NONE.
 */
#define MBEDTLS_ECP_DP_MAX     14

/*
 * Curve types
 */
typedef enum {
    MBEDTLS_ECP_TYPE_NONE = 0,
    MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS,    /* y^2 = x^3 + a x + b      */
    MBEDTLS_ECP_TYPE_MONTGOMERY,           /* y^2 = x^3 + a x^2 + x    */
} mbedtls_ecp_curve_type;

/**
 * Curve information, for use by other modules.
 *
 * The fields of this structure are part of the public API and can be
 * accessed directly by applications. Future versions of the library may
 * add extra fields or reorder existing fields.
 */
typedef struct mbedtls_ecp_curve_info {
    mbedtls_ecp_group_id grp_id;    /*!< An internal identifier. */
    uint16_t tls_id;                /*!< The TLS NamedCurve identifier. */
    uint16_t bit_size;              /*!< The curve size in bits. */
    const char *name;               /*!< A human-friendly name. */
} mbedtls_ecp_curve_info;

/**
 * \brief           The ECP point structure, in Jacobian coordinates.
 *
 * \note            All functions expect and return points satisfying
 *                  the following condition: <code>Z == 0</code> or
 *                  <code>Z == 1</code>. Other values of \p Z are
 *                  used only by internal functions.
 *                  The point is zero, or "at infinity", if <code>Z == 0</code>.
 *                  Otherwise, \p X and \p Y are its standard (affine)
 *                  coordinates.
 */
typedef struct mbedtls_ecp_point {
    mbedtls_mpi MBEDTLS_PRIVATE(X);          /*!< The X coordinate of the ECP point. */
    mbedtls_mpi MBEDTLS_PRIVATE(Y);          /*!< The Y coordinate of the ECP point. */
    mbedtls_mpi MBEDTLS_PRIVATE(Z);          /*!< The Z coordinate of the ECP point. */
}
mbedtls_ecp_point;

/*
 * default Mbed TLS elliptic curve arithmetic implementation
 *
 * (in case MBEDTLS_ECP_ALT is defined then the developer has to provide an
 * alternative implementation for the whole module and it will replace this
 * one.)
 */

/**
 * \brief           The ECP group structure.
 *
 * We consider two types of curve equations:
 * <ul><li>Short Weierstrass: <code>y^2 = x^3 + A x + B mod P</code>
 * (SEC1 + RFC-4492)</li>
 * <li>Montgomery: <code>y^2 = x^3 + A x^2 + x mod P</code> (Curve25519,
 * Curve448)</li></ul>
 * In both cases, the generator (\p G) for a prime-order subgroup is fixed.
 *
 * For Short Weierstrass, this subgroup is the whole curve, and its
 * cardinality is denoted by \p N. Our code requires that \p N is an
 * odd prime as mbedtls_ecp_mul() requires an odd number, and
 * mbedtls_ecdsa_sign() requires that it is prime for blinding purposes.
 *
 * The default implementation only initializes \p A without setting it to the
 * authentic value for curves with <code>A = -3</code>(SECP256R1, etc), in which
 * case you need to load \p A by yourself when using domain parameters directly,
 * for example:
 * \code
 * mbedtls_mpi_init(&A);
 * mbedtls_ecp_group_init(&grp);
 * CHECK_RETURN(mbedtls_ecp_group_load(&grp, grp_id));
 * if (mbedtls_ecp_group_a_is_minus_3(&grp)) {
 *     CHECK_RETURN(mbedtls_mpi_sub_int(&A, &grp.P, 3));
 * } else {
 *     CHECK_RETURN(mbedtls_mpi_copy(&A, &grp.A));
 * }
 *
 * do_something_with_a(&A);
 *
 * cleanup:
 * mbedtls_mpi_free(&A);
 * mbedtls_ecp_group_free(&grp);
 * \endcode
 *
 * For Montgomery curves, we do not store \p A, but <code>(A + 2) / 4</code>,
 * which is the quantity used in the formulas. Additionally, \p nbits is
 * not the size of \p N but the required size for private keys.
 *
 * If \p modp is NULL, reduction modulo \p P is done using a generic algorithm.
 * Otherwise, \p modp must point to a function that takes an \p mbedtls_mpi in the
 * range of [0, 2^(2*pbits)), and transforms it in-place to an integer which is
 * congruent mod \p P to the given MPI, is in the range [0, 2P), and has no more
 * non-zero limbs than P, so that it may be efficiently brought into the range
 * [0, P) by a single constant-time conditional subtraction.
 * It must return 0 on success and non-zero on failure.
 */
typedef struct mbedtls_ecp_group {
    mbedtls_ecp_group_id id;    /*!< An internal group identifier. */
    mbedtls_mpi P;              /*!< The prime modulus of the base field. */
    mbedtls_mpi A;              /*!< For Short Weierstrass: \p A in the equation. Note that
                                     \p A is not set to the authentic value in some cases.
                                     Refer to detailed description of ::mbedtls_ecp_group if
                                     using domain parameters in the structure.
                                     For Montgomery curves: <code>(A + 2) / 4</code>. */
    mbedtls_mpi B;              /*!< For Short Weierstrass: \p B in the equation.
                                     For Montgomery curves: unused. */
    mbedtls_ecp_point G;        /*!< The generator of the subgroup used. */
    mbedtls_mpi N;              /*!< The order of \p G. */
    size_t pbits;               /*!< The number of bits in \p P.*/
    size_t nbits;               /*!< For Short Weierstrass: The number of bits in \p P.
                                     For Montgomery curves: the number of bits in the
                                     private keys. */
    /* End of public fields */

    unsigned int MBEDTLS_PRIVATE(h);             /*!< \internal 1 if all the constants are static, 2 if only N and P are static */
    int(*MBEDTLS_PRIVATE(modp))(mbedtls_mpi *, MinTlsDiagnostics* diagnostics);  /*!< The function for fast pseudo-reduction
                                                    mod \p P (see above).*/
    int(*MBEDTLS_PRIVATE(t_pre))(mbedtls_ecp_point *, void *);   /*!< Unused. */
    int(*MBEDTLS_PRIVATE(t_post))(mbedtls_ecp_point *, void *);  /*!< Unused. */
    void *MBEDTLS_PRIVATE(t_data);               /*!< Unused. */
    mbedtls_ecp_point *MBEDTLS_PRIVATE(T);       /*!< Pre-computed points for ecp_mul_comb(). */
    size_t MBEDTLS_PRIVATE(T_size);              /*!< The number of dynamic allocated pre-computed points. */
}
mbedtls_ecp_group;

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h, or define them using the compiler command line.
 * \{
 */

#if !defined(MBEDTLS_ECP_WINDOW_SIZE)
/*
 * Maximum "window" size used for point multiplication.
 * Default: a point where higher memory usage yields diminishing performance
 *          returns.
 * Minimum value: 2. Maximum value: 7.
 *
 * Result is an array of at most ( 1 << ( MBEDTLS_ECP_WINDOW_SIZE - 1 ) )
 * points used for point multiplication. This value is directly tied to EC
 * peak memory usage, so decreasing it by one should roughly cut memory usage
 * by two (if large curves are in use).
 *
 * Reduction in size may reduce speed, but larger curves are impacted first.
 * Sample performances (in ECDHE handshakes/s, with FIXED_POINT_OPTIM = 1):
 *      w-size:     6       5       4       3       2
 *      521       145     141     135     120      97
 *      384       214     209     198     177     146
 *      256       320     320     303     262     226
 *      224       475     475     453     398     342
 *      192       640     640     633     587     476
 */
#define MBEDTLS_ECP_WINDOW_SIZE    4   /**< The maximum window size used. */
#endif /* MBEDTLS_ECP_WINDOW_SIZE */

#if !defined(MBEDTLS_ECP_FIXED_POINT_OPTIM)
/*
 * Trade code size for speed on fixed-point multiplication.
 *
 * This speeds up repeated multiplication of the generator (that is, the
 * multiplication in ECDSA signatures, and half of the multiplications in
 * ECDSA verification and ECDHE) by a factor roughly 3 to 4.
 *
 * For each n-bit Short Weierstrass curve that is enabled, this adds 4n bytes
 * of code size if n < 384 and 8n otherwise.
 *
 * Change this value to 0 to reduce code size.
 */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM  1   /**< Enable fixed-point speed-up. */
#endif /* MBEDTLS_ECP_FIXED_POINT_OPTIM */

/** \} name SECTION: Module settings */

/**
 * The maximum size of the groups, that is, of \c N and \c P.
 */
#define MBEDTLS_ECP_MAX_BITS 521

#define MBEDTLS_ECP_MAX_BYTES    ((MBEDTLS_ECP_MAX_BITS + 7) / 8)
#define MBEDTLS_ECP_MAX_PT_LEN   (2 * MBEDTLS_ECP_MAX_BYTES + 1)

#define MBEDTLS_ECP_BUDGET(ops)     /* no-op; for compatibility */

/* We want to declare restartable versions of existing functions anyway */
typedef void mbedtls_ecp_restart_ctx;

/**
 * \brief    The ECP key-pair structure.
 *
 * A generic key-pair that may be used for ECDSA and fixed ECDH, for example.
 *
 * \note    Members are deliberately in the same order as in the
 *          ::mbedtls_ecdsa_context structure.
 */
typedef struct mbedtls_ecp_keypair {
    mbedtls_ecp_group MBEDTLS_PRIVATE(grp);      /*!<  Elliptic curve and base point     */
    mbedtls_mpi MBEDTLS_PRIVATE(d);              /*!<  our secret value                  */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Q);        /*!<  our public value                  */
}
mbedtls_ecp_keypair;

/**
 * The uncompressed point format for Short Weierstrass curves
 * (MBEDTLS_ECP_DP_SECP_XXX and MBEDTLS_ECP_DP_BP_XXX).
 */
#define MBEDTLS_ECP_PF_UNCOMPRESSED    0
/**
 * The compressed point format for Short Weierstrass curves
 * (MBEDTLS_ECP_DP_SECP_XXX and MBEDTLS_ECP_DP_BP_XXX).
 *
 * \warning     While this format is supported for all concerned curves for
 *              writing, when it comes to parsing, it is not supported for all
 *              curves. Specifically, parsing compressed points on
 *              MBEDTLS_ECP_DP_SECP224R1 and MBEDTLS_ECP_DP_SECP224K1 is not
 *              supported.
 */
#define MBEDTLS_ECP_PF_COMPRESSED      1

/*
 * Some other constants from RFC 4492
 */
#define MBEDTLS_ECP_TLS_NAMED_CURVE    3   /**< The named_curve of ECCurveType. */

/*
 * Get the type of a curve
 */
mbedtls_ecp_curve_type mbedtls_ecp_get_type(const mbedtls_ecp_group *grp);

/**
 * \brief           This function retrieves the information defined in
 *                  mbedtls_ecp_curve_info() for all supported curves.
 *
 * \note            This function returns information about all curves
 *                  supported by the library. Some curves may not be
 *                  supported for all algorithms. Call mbedtls_ecdh_can_do()
 *                  or mbedtls_ecdsa_can_do() to check if a curve is
 *                  supported for ECDH or ECDSA.
 *
 * \return          A statically allocated array. The last entry is 0.
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_list(void);

/**
 * \brief           This function retrieves the list of internal group
 *                  identifiers of all supported curves in the order of
 *                  preference.
 *
 * \note            This function returns information about all curves
 *                  supported by the library. Some curves may not be
 *                  supported for all algorithms. Call mbedtls_ecdh_can_do()
 *                  or mbedtls_ecdsa_can_do() to check if a curve is
 *                  supported for ECDH or ECDSA.
 *
 * \return          A statically allocated array,
 *                  terminated with MBEDTLS_ECP_DP_NONE.
 */
const mbedtls_ecp_group_id *mbedtls_ecp_grp_id_list(void);

/**
 * \brief           This function retrieves curve information from a TLS
 *                  NamedCurve value.
 *
 * \param tls_id    An \c MBEDTLS_ECP_DP_XXX value.
 *
 * \return          The associated curve information on success.
 * \return          NULL on failure.
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_info_from_tls_id(uint16_t tls_id);

/**
 * \brief           This function initializes a point as zero.
 *
 * \param pt        The point to initialize.
 */
void mbedtls_ecp_point_init(mbedtls_ecp_point *pt);

/**
 * \brief           This function initializes an ECP group context
 *                  without loading any domain parameters.
 *
 * \note            After this function is called, domain parameters
 *                  for various ECP groups can be loaded through the
 *                  mbedtls_ecp_group_load() or mbedtls_ecp_tls_read_group()
 *                  functions.
 */
void mbedtls_ecp_group_init(mbedtls_ecp_group *grp);

/**
 * \brief           This function initializes a key pair as an invalid one.
 *
 * \param key       The key pair to initialize.
 */
void mbedtls_ecp_keypair_init(mbedtls_ecp_keypair *key);

/**
 * \brief           This function frees the components of a point.
 *
 * \param pt        The point to free.
 */
void mbedtls_ecp_point_free(mbedtls_ecp_point *pt);

/**
 * \brief           This function frees the components of an ECP group.
 *
 * \param grp       The group to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP group.
 */
void mbedtls_ecp_group_free(mbedtls_ecp_group *grp);

/**
 * \brief           This function frees the components of a key pair.
 *
 * \param key       The key pair to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP key pair.
 */
void mbedtls_ecp_keypair_free(mbedtls_ecp_keypair *key);

/**
 * \brief           This function copies the contents of point \p Q into
 *                  point \p P.
 *
 * \param P         The destination point. This must be initialized.
 * \param Q         The source point. This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          Another negative error code for other kinds of failure.
 */
int mbedtls_ecp_copy(mbedtls_ecp_point *P, const mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function copies the contents of group \p src into
 *                  group \p dst.
 *
 * \param dst       The destination group. This must be initialized.
 * \param src       The source group. This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_group_copy(mbedtls_ecp_group *dst,
                           const mbedtls_ecp_group *src, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function sets a point to the point at infinity.
 *
 * \param pt        The point to set. This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_set_zero(mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function checks if a point is the point at infinity.
 *
 * \param pt        The point to test. This must be initialized.
 *
 * \return          \c 1 if the point is zero.
 * \return          \c 0 if the point is non-zero.
 * \return          A negative error code on failure.
 */
int mbedtls_ecp_is_zero(mbedtls_ecp_point *pt);

/**
 * \brief           This function exports a point into unsigned binary data.
 *
 * \param grp       The group to which the point should belong.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param P         The point to export. This must be initialized.
 * \param format    The point format. This must be either
 *                  #MBEDTLS_ECP_PF_COMPRESSED or #MBEDTLS_ECP_PF_UNCOMPRESSED.
 *                  (For groups without these formats, this parameter is
 *                  ignored. But it still has to be either of the above
 *                  values.)
 * \param olen      The address at which to store the length of
 *                  the output in Bytes. This must not be \c NULL.
 * \param buf       The output buffer. This must be a writable buffer
 *                  of length \p buflen Bytes.
 * \param buflen    The length of the output buffer \p buf in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL if the output buffer
 *                  is too small to hold the point.
 * \return          #MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if the point format
 *                  or the export for the given group is not implemented.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_point_write_binary(const mbedtls_ecp_group *grp,
                                   const mbedtls_ecp_point *P,
                                   int format, size_t *olen,
                                   unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function imports a point from unsigned binary data.
 *
 * \note            This function does not check that the point actually
 *                  belongs to the given group, see mbedtls_ecp_check_pubkey()
 *                  for that.
 *
 * \note            For compressed points, see #MBEDTLS_ECP_PF_COMPRESSED for
 *                  limitations.
 *
 * \param grp       The group to which the point should belong.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param P         The destination context to import the point to.
 *                  This must be initialized.
 * \param buf       The input buffer. This must be a readable buffer
 *                  of length \p ilen Bytes.
 * \param ilen      The length of the input buffer \p buf in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if the input is invalid.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          #MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if the import for the
 *                  given group is not implemented.
 */
int mbedtls_ecp_point_read_binary(const mbedtls_ecp_group *grp,
                                  mbedtls_ecp_point *P,
                                  const unsigned char *buf, size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function imports a point from a TLS ECPoint record.
 *
 * \note            On function return, \p *buf is updated to point immediately
 *                  after the ECPoint record.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param pt        The destination point.
 * \param buf       The address of the pointer to the start of the input buffer.
 * \param len       The length of the buffer.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_MPI_XXX error code on initialization
 *                  failure.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if input is invalid.
 */
int mbedtls_ecp_tls_read_point(const mbedtls_ecp_group *grp,
                               mbedtls_ecp_point *pt,
                               const unsigned char **buf, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function exports a point as a TLS ECPoint record
 *                  defined in RFC 4492, Section 5.4.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param pt        The point to be exported. This must be initialized.
 * \param format    The point format to use. This must be either
 *                  #MBEDTLS_ECP_PF_COMPRESSED or #MBEDTLS_ECP_PF_UNCOMPRESSED.
 * \param olen      The address at which to store the length in Bytes
 *                  of the data written.
 * \param buf       The target buffer. This must be a writable buffer of
 *                  length \p blen Bytes.
 * \param blen      The length of the target buffer \p buf in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if the input is invalid.
 * \return          #MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL if the target buffer
 *                  is too small to hold the exported point.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_tls_write_point(const mbedtls_ecp_group *grp,
                                const mbedtls_ecp_point *pt,
                                int format, size_t *olen,
                                unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function sets up an ECP group context
 *                  from a standardized set of domain parameters.
 *
 * \note            The index should be a value of the NamedCurve enum,
 *                  as defined in <em>RFC-4492: Elliptic Curve Cryptography
 *                  (ECC) Cipher Suites for Transport Layer Security (TLS)</em>,
 *                  usually in the form of an \c MBEDTLS_ECP_DP_XXX macro.
 *
 * \param grp       The group context to setup. This must be initialized.
 * \param id        The identifier of the domain parameter set to load.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if \p id doesn't
 *                  correspond to a known group.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_group_load(mbedtls_ecp_group *grp, mbedtls_ecp_group_id id, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function extracts an elliptic curve group ID from a
 *                  TLS ECParameters record as defined in RFC 4492, Section 5.4.
 *
 * \note            The read pointer \p buf is updated to point right after
 *                  the ECParameters record on exit.
 *
 * \param grp       The address at which to store the group id.
 *                  This must not be \c NULL.
 * \param buf       The address of the pointer to the start of the input buffer.
 * \param len       The length of the input buffer \c *buf in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if input is invalid.
 * \return          #MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if the group is not
 *                  recognized.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_tls_read_group_id(mbedtls_ecp_group_id *grp,
                                  const unsigned char **buf,
                                  size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function performs a scalar multiplication of a point
 *                  by an integer: \p R = \p m * \p P.
 *
 *                  It is not thread-safe to use same group in multiple threads.
 *
 * \note            To prevent timing attacks, this function
 *                  executes the exact same sequence of base-field
 *                  operations for any valid \p m. It avoids any if-branch or
 *                  array index depending on the value of \p m. It also uses
 *                  \p f_rng to randomize some intermediate results.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param R         The point in which to store the result of the calculation.
 *                  This must be initialized.
 * \param m         The integer by which to multiply. This must be initialized.
 * \param P         The point to multiply. This must be initialized.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be \c
 *                  NULL if \p f_rng doesn't need a context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_INVALID_KEY if \p m is not a valid private
 *                  key, or \p P is not a valid public key.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_mul(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                    const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                    mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function performs multiplication of a point by
 *                  an integer: \p R = \p m * \p P in a restartable way.
 *
 * \see             mbedtls_ecp_mul()
 *
 * \note            This function does the same as \c mbedtls_ecp_mul(), but
 *                  it can return early and restart according to the limit set
 *                  with \c mbedtls_ecp_set_max_ops() to reduce blocking.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param R         The point in which to store the result of the calculation.
 *                  This must be initialized.
 * \param m         The integer by which to multiply. This must be initialized.
 * \param P         The point to multiply. This must be initialized.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be \c
 *                  NULL if \p f_rng doesn't need a context.
 * \param rs_ctx    The restart context (NULL disables restart).
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_INVALID_KEY if \p m is not a valid private
 *                  key, or \p P is not a valid public key.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                                const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                                mbedtls_f_rng_t *f_rng, void *p_rng,
                                mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
/**
 * \brief           This function checks if domain parameter A of the curve is
 *                  \c -3.
 *
 * \note            This function is only defined for short Weierstrass curves.
 *                  It may not be included in builds without any short
 *                  Weierstrass curve.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 *
 * \return          \c 1 if <code>A = -3</code>.
 * \return          \c 0 Otherwise.
 */
static inline int mbedtls_ecp_group_a_is_minus_3(const mbedtls_ecp_group *grp)
{
    return grp->A.MBEDTLS_PRIVATE(p) == NULL;
}

/**
 * \brief           This function performs multiplication and addition of two
 *                  points by integers: \p R = \p m * \p P + \p n * \p Q in a
 *                  restartable way.
 *
 * \see             \c mbedtls_ecp_muladd()
 *
 * \note            This function works the same as \c mbedtls_ecp_muladd(),
 *                  but it can return early and restart according to the limit
 *                  set with \c mbedtls_ecp_set_max_ops() to reduce blocking.
 *
 * \note            This function is only defined for short Weierstrass curves.
 *                  It may not be included in builds without any short
 *                  Weierstrass curve.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param R         The point in which to store the result of the calculation.
 *                  This must be initialized.
 * \param m         The integer by which to multiply \p P.
 *                  This must be initialized.
 * \param P         The point to multiply by \p m. This must be initialized.
 * \param n         The integer by which to multiply \p Q.
 *                  This must be initialized.
 * \param Q         The point to be multiplied by \p n.
 *                  This must be initialized.
 * \param rs_ctx    The restart context (NULL disables restart).
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_INVALID_KEY if \p m or \p n are not
 *                  valid private keys, or \p P or \p Q are not valid public
 *                  keys.
 * \return          #MBEDTLS_ERR_MPI_ALLOC_FAILED on memory-allocation failure.
 * \return          #MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if \p grp does not
 *                  designate a short Weierstrass curve.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_muladd_restartable(
    mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
    const mbedtls_mpi *m, const mbedtls_ecp_point *P,
    const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
    mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

/**
 * \brief           This function checks that a point is a valid public key
 *                  on this curve.
 *
 *                  It only checks that the point is non-zero, has
 *                  valid coordinates and lies on the curve. It does not verify
 *                  that it is indeed a multiple of \c G. This additional
 *                  check is computationally more expensive, is not required
 *                  by standards, and should not be necessary if the group
 *                  used has a small cofactor. In particular, it is useless for
 *                  the NIST groups which all have a cofactor of 1.
 *
 * \note            This function uses bare components rather than an
 *                  ::mbedtls_ecp_keypair structure, to ease use with other
 *                  structures, such as ::mbedtls_ecdh_context or
 *                  ::mbedtls_ecdsa_context.
 *
 * \param grp       The ECP group the point should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param pt        The point to check. This must be initialized.
 *
 * \return          \c 0 if the point is a valid public key.
 * \return          #MBEDTLS_ERR_ECP_INVALID_KEY if the point is not
 *                  a valid public key for the given curve.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
                             const mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function checks that an \c mbedtls_mpi is a
 *                  valid private key for this curve.
 *
 * \note            This function uses bare components rather than an
 *                  ::mbedtls_ecp_keypair structure to ease use with other
 *                  structures, such as ::mbedtls_ecdh_context or
 *                  ::mbedtls_ecdsa_context.
 *
 * \param grp       The ECP group the private key should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param d         The integer to check. This must be initialized.
 *
 * \return          \c 0 if the point is a valid private key.
 * \return          #MBEDTLS_ERR_ECP_INVALID_KEY if the point is not a valid
 *                  private key for the given curve.
 * \return          Another negative error code on other kinds of failure.
 */
int mbedtls_ecp_check_privkey(const mbedtls_ecp_group *grp,
                              const mbedtls_mpi *d, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function generates a private key.
 *
 * \param grp       The ECP group to generate a private key for.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param d         The destination MPI (secret part). This must be initialized.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG parameter to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX or \c MBEDTLS_MPI_XXX error code
 *                  on failure.
 */
int mbedtls_ecp_gen_privkey(const mbedtls_ecp_group *grp,
                            mbedtls_mpi *d,
                            mbedtls_f_rng_t *f_rng,
                            void *p_rng, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

#endif

/**
 * \brief           This function checks that the keypair objects
 *                  \p pub and \p prv have the same group and the
 *                  same public point, and that the private key in
 *                  \p prv is consistent with the public key.
 *
 * \param pub       The keypair structure holding the public key. This
 *                  must be initialized. If it contains a private key, that
 *                  part is ignored.
 * \param prv       The keypair structure holding the full keypair.
 *                  This must be initialized.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be \c
 *                  NULL if \p f_rng doesn't need a context.
 *
 * \return          \c 0 on success, meaning that the keys are valid and match.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if the keys are invalid or do not match.
 * \return          An \c MBEDTLS_ERR_ECP_XXX or an \c MBEDTLS_ERR_MPI_XXX
 *                  error code on calculation failure.
 */
int mbedtls_ecp_check_pub_priv(
    const mbedtls_ecp_keypair *pub, const mbedtls_ecp_keypair *prv,
    mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* ecp.h */


/*
 * Mbed TLS supports two formats for ECDH contexts (#mbedtls_ecdh_context
 * defined in `ecdh.h`). For most applications, the choice of format makes
 * no difference, since all library functions can work with either format,
 * except that the new format is incompatible with MBEDTLS_ECP_RESTARTABLE.

 * The new format used when this option is disabled is smaller
 * (56 bytes on a 32-bit platform). In future versions of the library, it
 * will support alternative implementations of ECDH operations.
 * The new format is incompatible with applications that access
 * context fields directly and with restartable ECP operations.
 */

#undef MBEDTLS_ECDH_LEGACY_CONTEXT

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines the source of the imported EC key.
 */
typedef enum {
    MBEDTLS_ECDH_OURS,   /**< Our key. */
    MBEDTLS_ECDH_THEIRS, /**< The key of the peer. */
} mbedtls_ecdh_side;

#if !defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
/**
 * Defines the ECDH implementation used.
 *
 * Later versions of the library may add new variants, therefore users should
 * not make any assumptions about them.
 */
typedef enum {
    MBEDTLS_ECDH_VARIANT_NONE = 0,   /*!< Implementation not defined. */
    MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0,/*!< The default Mbed TLS implementation */
} mbedtls_ecdh_variant;

/**
 * The context used by the default ECDH implementation.
 *
 * Later versions might change the structure of this context, therefore users
 * should not make any assumptions about the structure of
 * mbedtls_ecdh_context_mbed.
 */
typedef struct mbedtls_ecdh_context_mbed {
    mbedtls_ecp_group MBEDTLS_PRIVATE(grp);   /*!< The elliptic curve used. */
    mbedtls_mpi MBEDTLS_PRIVATE(d);           /*!< The private key. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Q);     /*!< The public key. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Qp);    /*!< The value of the public key of the peer. */
    mbedtls_mpi MBEDTLS_PRIVATE(z);           /*!< The shared secret. */
} mbedtls_ecdh_context_mbed;
#endif

/**
 *
 * \warning         Performing multiple operations concurrently on the same
 *                  ECDSA context is not supported; objects of this type
 *                  should not be shared between multiple threads.
 * \brief           The ECDH context structure.
 */
typedef struct mbedtls_ecdh_context {
#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    mbedtls_ecp_group MBEDTLS_PRIVATE(grp);   /*!< The elliptic curve used. */
    mbedtls_mpi MBEDTLS_PRIVATE(d);           /*!< The private key. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Q);     /*!< The public key. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Qp);    /*!< The value of the public key of the peer. */
    mbedtls_mpi MBEDTLS_PRIVATE(z);           /*!< The shared secret. */
    int MBEDTLS_PRIVATE(point_format);        /*!< The format of point export in TLS messages. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Vi);    /*!< The blinding value. */
    mbedtls_ecp_point MBEDTLS_PRIVATE(Vf);    /*!< The unblinding value. */
    mbedtls_mpi MBEDTLS_PRIVATE(_d);          /*!< The previous \p d. */
#else
    uint8_t MBEDTLS_PRIVATE(point_format);       /*!< The format of point export in TLS messages
                                                    as defined in RFC 4492. */
    mbedtls_ecp_group_id MBEDTLS_PRIVATE(grp_id);/*!< The elliptic curve used. */
    mbedtls_ecdh_variant MBEDTLS_PRIVATE(var);   /*!< The ECDH implementation/structure used. */
    union {
        mbedtls_ecdh_context_mbed   MBEDTLS_PRIVATE(mbed_ecdh);
    } MBEDTLS_PRIVATE(ctx);                      /*!< Implementation-specific context. The
                                                    context in use is specified by the \c var
                                                    field. */
#endif /* MBEDTLS_ECDH_LEGACY_CONTEXT */
}
mbedtls_ecdh_context;

/**
 * \brief           This function generates an ECDH keypair on an elliptic
 *                  curve.
 *
 *                  This function performs the first of two core computations
 *                  implemented during the ECDH key exchange. The second core
 *                  computation is performed by mbedtls_ecdh_compute_shared().
 *
 * \see             ecp.h
 *
 * \param grp       The ECP group to use. This must be initialized and have
 *                  domain parameters loaded, for example through
 *                  mbedtls_ecp_load() or mbedtls_ecp_tls_read_group().
 * \param d         The destination MPI (private key).
 *                  This must be initialized.
 * \param Q         The destination point (public key).
 *                  This must be initialized.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL in case \p f_rng doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          Another \c MBEDTLS_ERR_ECP_XXX or
 *                  \c MBEDTLS_MPI_XXX error code on failure.
 */
int mbedtls_ecdh_gen_public(mbedtls_ecp_group *grp, mbedtls_mpi *d, mbedtls_ecp_point *Q,
                            mbedtls_f_rng_t *f_rng,
                            void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function computes the shared secret.
 *
 *                  This function performs the second of two core computations
 *                  implemented during the ECDH key exchange. The first core
 *                  computation is performed by mbedtls_ecdh_gen_public().
 *
 * \see             ecp.h
 *
 * \note            If \p f_rng is not NULL, it is used to implement
 *                  countermeasures against side-channel attacks.
 *                  For more information, see mbedtls_ecp_mul().
 *
 * \param grp       The ECP group to use. This must be initialized and have
 *                  domain parameters loaded, for example through
 *                  mbedtls_ecp_load() or mbedtls_ecp_tls_read_group().
 * \param z         The destination MPI (shared secret).
 *                  This must be initialized.
 * \param Q         The public key from another party.
 *                  This must be initialized.
 * \param d         Our secret exponent (private key).
 *                  This must be initialized.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't need a
 *                  context argument.
 *
 * \return          \c 0 on success.
 * \return          Another \c MBEDTLS_ERR_ECP_XXX or
 *                  \c MBEDTLS_MPI_XXX error code on failure.
 */
int mbedtls_ecdh_compute_shared(mbedtls_ecp_group *grp, mbedtls_mpi *z,
                                const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                                mbedtls_f_rng_t *f_rng,
                                void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function initializes an ECDH context.
 *
 * \param ctx       The ECDH context to initialize. This must not be \c NULL.
 */
void mbedtls_ecdh_init(mbedtls_ecdh_context *ctx);

/**
 * \brief           This function sets up the ECDH context with the information
 *                  given.
 *
 *                  This function should be called after mbedtls_ecdh_init() but
 *                  before mbedtls_ecdh_make_params(). There is no need to call
 *                  this function before mbedtls_ecdh_read_params().
 *
 *                  This is the first function used by a TLS server for ECDHE
 *                  ciphersuites.
 *
 * \param ctx       The ECDH context to set up. This must be initialized.
 * \param grp_id    The group id of the group to set up the context for.
 *
 * \return          \c 0 on success.
 */
int mbedtls_ecdh_setup(mbedtls_ecdh_context *ctx,
                       mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function frees a context.
 *
 * \param ctx       The context to free. This may be \c NULL, in which
 *                  case this function does nothing. If it is not \c NULL,
 *                  it must point to an initialized ECDH context.
 */
void mbedtls_ecdh_free(mbedtls_ecdh_context *ctx);

/**
 * \brief           This function parses the ECDHE parameters in a
 *                  TLS ServerKeyExchange handshake message.
 *
 * \note            In a TLS handshake, this is the how the client
 *                  sets up its ECDHE context from the server's public
 *                  ECDHE key material.
 *
 * \see             ecp.h
 *
 * \param ctx       The ECDHE context to use. This must be initialized.
 * \param buf       On input, \c *buf must be the start of the input buffer.
 *                  On output, \c *buf is updated to point to the end of the
 *                  data that has been read. On success, this is the first byte
 *                  past the end of the ServerKeyExchange parameters.
 *                  On error, this is the point at which an error has been
 *                  detected, which is usually not useful except to debug
 *                  failures.
 * \param end       The end of the input buffer.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX error code on failure.
 *
 */
int mbedtls_ecdh_read_params(mbedtls_ecdh_context *ctx,
                             const unsigned char **buf,
                             const unsigned char *end, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function generates a public key and exports it
 *                  as a TLS ClientKeyExchange payload.
 *
 *                  This is the second function used by a TLS client for ECDH(E)
 *                  ciphersuites.
 *
 * \see             ecp.h
 *
 * \param ctx       The ECDH context to use. This must be initialized
 *                  and bound to a group, the latter usually by
 *                  mbedtls_ecdh_read_params().
 * \param olen      The address at which to store the number of Bytes written.
 *                  This must not be \c NULL.
 * \param buf       The destination buffer. This must be a writable buffer
 *                  of length \p blen Bytes.
 * \param blen      The size of the destination buffer \p buf in Bytes.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL in case \p f_rng doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_ecdh_make_public(mbedtls_ecdh_context *ctx, size_t *olen,
                             unsigned char *buf, size_t blen,
                             mbedtls_f_rng_t *f_rng,
                             void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function derives and exports the shared secret.
 *
 *                  This is the last function used by both TLS client
 *                  and servers.
 *
 * \note            If \p f_rng is not NULL, it is used to implement
 *                  countermeasures against side-channel attacks.
 *                  For more information, see mbedtls_ecp_mul().
 *
 * \see             ecp.h

 * \param ctx       The ECDH context to use. This must be initialized
 *                  and have its own private key generated and the peer's
 *                  public key imported.
 * \param olen      The address at which to store the total number of
 *                  Bytes written on success. This must not be \c NULL.
 * \param buf       The buffer to write the generated shared key to. This
 *                  must be a writable buffer of size \p blen Bytes.
 *                  A sufficient size is given by #MBEDTLS_ECP_MAX_BYTES.
 * \param blen      The length of the destination buffer \p buf in Bytes.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context. This may be \c NULL if \p f_rng
 *                  doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_ecdh_calc_secret(mbedtls_ecdh_context *ctx, size_t *olen,
                             unsigned char *buf, size_t blen,
                             mbedtls_f_rng_t *f_rng,
                             void *p_rng, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* ecdh.h */

/* ===== Retained aes.h ===== */

/**
 * \file aes.h
 *
 * \brief   This file contains AES definitions and functions.
 *
 *          The Advanced Encryption Standard (AES) specifies a FIPS-approved
 *          cryptographic algorithm that can be used to protect electronic
 *          data.
 *
 *          The AES algorithm is a symmetric block cipher that can
 *          encrypt and decrypt information. For more information, see
 *          <em>FIPS Publication 197: Advanced Encryption Standard</em> and
 *          <em>ISO/IEC 18033-2:2006: Information technology -- Security
 *          techniques -- Encryption algorithms -- Part 2: Asymmetric
 *          ciphers</em>.
 *
 *          The AES-XTS block mode is standardized by NIST SP 800-38E
 *          <https://nvlpubs.nist.gov/nistpubs/legacy/sp/nistspecialpublication800-38e.pdf>
 *          and described in detail by IEEE P1619
 *          <https://ieeexplore.ieee.org/servlet/opac?punumber=4375278>.
 */

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/aes.h.
 */

#ifndef MBEDTLS_AES_H
#define MBEDTLS_AES_H


#include <stddef.h>
#include <stdint.h>

/* padlock.c and aesni.c rely on these values! */
#define MBEDTLS_AES_ENCRYPT     1 /**< AES encryption. */
#define MBEDTLS_AES_DECRYPT     0 /**< AES decryption. */

/* Error codes in range 0x0020-0x0022 */
/** Invalid key length. */
#define MBEDTLS_ERR_AES_INVALID_KEY_LENGTH                -0x0020
/** Invalid data input length. */
#define MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH              -0x0022

/* Error codes in range 0x0021-0x0025 */
/** Invalid input data. */
#define MBEDTLS_ERR_AES_BAD_INPUT_DATA                    -0x0021

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_AES_ALT)
// Regular implementation
//

/**
 * \brief The AES context-type definition.
 */
typedef struct mbedtls_aes_context {
    int MBEDTLS_PRIVATE(nr);                     /*!< The number of rounds. */
    size_t MBEDTLS_PRIVATE(rk_offset);           /*!< The offset in array elements to AES
                                                    round keys in the buffer. */
#if defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH) && !defined(MBEDTLS_PADLOCK_C)
    uint32_t MBEDTLS_PRIVATE(buf)[44];           /*!< Aligned data buffer to hold
                                                    10 round keys for 128-bit case. */
#else
    uint32_t MBEDTLS_PRIVATE(buf)[68];           /*!< Unaligned data buffer. This buffer can
                                                    hold 32 extra Bytes, which can be used for
                                                    one of the following purposes:
                                                    <ul><li>Alignment if VIA padlock is
                                                    used.</li>
                                                    <li>Simplifying key expansion in the 256-bit
                                                    case by generating an extra round key.
                                                    </li></ul> */
#endif /* MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH && !MBEDTLS_PADLOCK_C */
}
mbedtls_aes_context;

#else
#include "aes_alt.h"
#endif /* MBEDTLS_AES_ALT */

/**
 * \brief          This function initializes the specified AES context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \param ctx      The AES context to initialize. This must not be \c NULL.
 */
void mbedtls_aes_init(mbedtls_aes_context *ctx);

/**
 * \brief          This function releases and clears the specified AES context.
 *
 * \param ctx      The AES context to clear.
 *                 If this is \c NULL, this function does nothing.
 *                 Otherwise, the context must have been at least initialized.
 */
void mbedtls_aes_free(mbedtls_aes_context *ctx);

/**
 * \brief          This function sets the encryption key.
 *
 * \param ctx      The AES context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed in bits. Valid options are:
 *                 <ul><li>128 bits</li>
 *                 <li>192 bits</li>
 *                 <li>256 bits</li></ul>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_AES_INVALID_KEY_LENGTH on failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
/**
 * \brief          This function sets the decryption key.
 *
 * \param ctx      The AES context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed. Valid options are:
 *                 <ul><li>128 bits</li>
 *                 <li>192 bits</li>
 *                 <li>256 bits</li></ul>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_AES_INVALID_KEY_LENGTH on failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits, MinTlsDiagnostics* diagnostics);
#endif /* !MBEDTLS_BLOCK_CIPHER_NO_DECRYPT */

/**
 * \brief          This function performs an AES single-block encryption or
 *                 decryption operation.
 *
 *                 It performs the operation defined in the \p mode parameter
 *                 (encrypt or decrypt), on the input data buffer defined in
 *                 the \p input parameter.
 *
 *                 mbedtls_aes_init(), and either mbedtls_aes_setkey_enc() or
 *                 mbedtls_aes_setkey_dec() must be called before the first
 *                 call to this API with the same context.
 *
 * \param ctx      The AES context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The AES operation: #MBEDTLS_AES_ENCRYPT or
 *                 #MBEDTLS_AES_DECRYPT.
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \c 16 Bytes long.
 * \param output   The buffer where the output data will be written.
 *                 It must be writeable and at least \c 16 Bytes long.

 * \return         \c 0 on success.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx,
                          int mode,
                          const unsigned char input[16],
                          unsigned char output[16], MinTlsDiagnostics* diagnostics);

/**
 * \brief           Internal AES block encryption function. This is only
 *                  exposed to allow overriding it using
 *                  \c MBEDTLS_AES_ENCRYPT_ALT.
 *
 * \param ctx       The AES context to use for encryption.
 * \param input     The plaintext block.
 * \param output    The output (ciphertext) block.
 *
 * \return          \c 0 on success.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_internal_aes_encrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16]);

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
/**
 * \brief           Internal AES block decryption function. This is only
 *                  exposed to allow overriding it using see
 *                  \c MBEDTLS_AES_DECRYPT_ALT.
 *
 * \param ctx       The AES context to use for decryption.
 * \param input     The ciphertext block.
 * \param output    The output (plaintext) block.
 *
 * \return          \c 0 on success.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_internal_aes_decrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16]);
#endif /* !MBEDTLS_BLOCK_CIPHER_NO_DECRYPT */

#ifdef __cplusplus
}
#endif

#endif /* aes.h */

/* ===== Retained asn1.h ===== */

/**
 * \file asn1.h
 *
 * \brief Generic ASN.1 parsing
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/asn1.h.
 */
#ifndef MBEDTLS_ASN1_H
#define MBEDTLS_ASN1_H


#include <stddef.h>


/**
 * \addtogroup asn1_module
 * \{
 */

/**
 * \name ASN1 Error codes
 * These error codes are combined with other error codes for
 * higher error granularity.
 * e.g. X.509 and PKCS #7 error codes
 * ASN1 is a standard to specify data structures.
 * \{
 */
/** Out of data when parsing an ASN1 data structure. */
#define MBEDTLS_ERR_ASN1_OUT_OF_DATA                      -0x0060
/** ASN1 tag was of an unexpected value. */
#define MBEDTLS_ERR_ASN1_UNEXPECTED_TAG                   -0x0062
/** Error when trying to determine the length or invalid length. */
#define MBEDTLS_ERR_ASN1_INVALID_LENGTH                   -0x0064
/** Actual length differs from expected length. */
#define MBEDTLS_ERR_ASN1_LENGTH_MISMATCH                  -0x0066
/** Data is invalid. */
#define MBEDTLS_ERR_ASN1_INVALID_DATA                     -0x0068
/** Memory allocation failed */
#define MBEDTLS_ERR_ASN1_ALLOC_FAILED                     -0x006A
/** Buffer too small when writing ASN.1 data structure. */
#define MBEDTLS_ERR_ASN1_BUF_TOO_SMALL                    -0x006C

/** \} name ASN1 Error codes */

/**
 * \name DER constants
 * These constants comply with the DER encoded ASN.1 type tags.
 * DER encoding uses hexadecimal representation.
 * An example DER sequence is:\n
 * - 0x02 -- tag indicating INTEGER
 * - 0x01 -- length in octets
 * - 0x05 -- value
 * Such sequences are typically read into \c ::mbedtls_x509_buf.
 * \{
 */
#define MBEDTLS_ASN1_BOOLEAN                 0x01
#define MBEDTLS_ASN1_INTEGER                 0x02
#define MBEDTLS_ASN1_BIT_STRING              0x03
#define MBEDTLS_ASN1_OCTET_STRING            0x04
#define MBEDTLS_ASN1_NULL                    0x05
#define MBEDTLS_ASN1_OID                     0x06
#define MBEDTLS_ASN1_ENUMERATED              0x0A
#define MBEDTLS_ASN1_UTF8_STRING             0x0C
#define MBEDTLS_ASN1_SEQUENCE                0x10
#define MBEDTLS_ASN1_SET                     0x11
#define MBEDTLS_ASN1_PRINTABLE_STRING        0x13
#define MBEDTLS_ASN1_T61_STRING              0x14
#define MBEDTLS_ASN1_IA5_STRING              0x16
#define MBEDTLS_ASN1_UTC_TIME                0x17
#define MBEDTLS_ASN1_GENERALIZED_TIME        0x18
#define MBEDTLS_ASN1_UNIVERSAL_STRING        0x1C
#define MBEDTLS_ASN1_BMP_STRING              0x1E
#define MBEDTLS_ASN1_PRIMITIVE               0x00
#define MBEDTLS_ASN1_CONSTRUCTED             0x20
#define MBEDTLS_ASN1_CONTEXT_SPECIFIC        0x80

/* Slightly smaller way to check if tag is a string tag
 * compared to canonical implementation. */
#define MBEDTLS_ASN1_IS_STRING_TAG(tag)                                \
    ((unsigned int) (tag) < 32u && (                                   \
         ((1u << (tag)) & ((1u << MBEDTLS_ASN1_BMP_STRING)       |     \
                           (1u << MBEDTLS_ASN1_UTF8_STRING)      |     \
                           (1u << MBEDTLS_ASN1_T61_STRING)       |     \
                           (1u << MBEDTLS_ASN1_IA5_STRING)       |     \
                           (1u << MBEDTLS_ASN1_UNIVERSAL_STRING) |     \
                           (1u << MBEDTLS_ASN1_PRINTABLE_STRING))) != 0))

/*
 * Bit masks for each of the components of an ASN.1 tag as specified in
 * ITU X.690 (08/2015), section 8.1 "General rules for encoding",
 * paragraph 8.1.2.2:
 *
 * Bit  8     7   6   5          1
 *     +-------+-----+------------+
 *     | Class | P/C | Tag number |
 *     +-------+-----+------------+
 */
#define MBEDTLS_ASN1_TAG_CLASS_MASK          0xC0
#define MBEDTLS_ASN1_TAG_PC_MASK             0x20
#define MBEDTLS_ASN1_TAG_VALUE_MASK          0x1F

/** \} name DER constants */

/** Returns the size of the binary string, without the trailing \\0 */
#define MBEDTLS_OID_SIZE(x) (sizeof(x) - 1)

/**
 * Compares an mbedtls_asn1_buf structure to a reference OID.
 *
 * Only works for 'defined' oid_str values (MBEDTLS_OID_HMAC_SHA1), you cannot use a
 * 'unsigned char *oid' here!
 */
#define MBEDTLS_OID_CMP(oid_str, oid_buf)                                   \
    ((MBEDTLS_OID_SIZE(oid_str) != (oid_buf)->len) ||                \
     memcmp((oid_str), (oid_buf)->p, (oid_buf)->len) != 0)

#define MBEDTLS_OID_CMP_RAW(oid_str, oid_buf, oid_buf_len)              \
    ((MBEDTLS_OID_SIZE(oid_str) != (oid_buf_len)) ||             \
     memcmp((oid_str), (oid_buf), (oid_buf_len)) != 0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name Functions to parse ASN.1 data structures
 * \{
 */

/**
 * Type-length-value structure that allows for ASN1 using DER.
 */
typedef struct mbedtls_asn1_buf {
    int tag;                /**< ASN1 type, e.g. MBEDTLS_ASN1_UTF8_STRING. */
    size_t len;             /**< ASN1 length, in octets. */
    unsigned char *p;       /**< ASN1 data, e.g. in ASCII. */
}
mbedtls_asn1_buf;

/**
 * Container for ASN1 bit strings.
 */
typedef struct mbedtls_asn1_bitstring {
    size_t len;                 /**< ASN1 length, in octets. */
    unsigned char unused_bits;  /**< Number of unused bits at the end of the string */
    unsigned char *p;           /**< Raw ASN1 data for the bit string */
}
mbedtls_asn1_bitstring;

/**
 * Container for a sequence of ASN.1 items
 */
typedef struct mbedtls_asn1_sequence {
    mbedtls_asn1_buf buf;                   /**< Buffer containing the given ASN.1 item. */

    /** The next entry in the sequence.
     *
     * The details of memory management for sequences are not documented and
     * may change in future versions. Set this field to \p NULL when
     * initializing a structure, and do not modify it except via Mbed TLS
     * library functions.
     */
    struct mbedtls_asn1_sequence *next;
}
mbedtls_asn1_sequence;

/**
 * Container for a sequence or list of 'named' ASN.1 data items
 */
typedef struct mbedtls_asn1_named_data {
    mbedtls_asn1_buf oid;                   /**< The object identifier. */
    mbedtls_asn1_buf val;                   /**< The named value. */

    /** The next entry in the sequence.
     *
     * The details of memory management for named data sequences are not
     * documented and may change in future versions. Set this field to \p NULL
     * when initializing a structure, and do not modify it except via Mbed TLS
     * library functions.
     */
    struct mbedtls_asn1_named_data *next;

    /** Merge next item into the current one?
     *
     * This field exists for the sake of Mbed TLS's X.509 certificate parsing
     * code and may change in future versions of the library.
     */
    unsigned char MBEDTLS_PRIVATE(next_merged);
}
mbedtls_asn1_named_data;

/**
 * \brief       Get the length of an ASN.1 element.
 *              Updates the pointer to immediately behind the length.
 *
 * \param p     On entry, \c *p points to the first byte of the length,
 *              i.e. immediately after the tag.
 *              On successful completion, \c *p points to the first byte
 *              after the length, i.e. the first byte of the content.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param len   On successful completion, \c *len contains the length
 *              read from the ASN.1 input.
 *
 * \return      0 if successful.
 * \return      #MBEDTLS_ERR_ASN1_OUT_OF_DATA if the ASN.1 element
 *              would end beyond \p end.
 * \return      #MBEDTLS_ERR_ASN1_INVALID_LENGTH if the length is unparsable.
 */
int mbedtls_asn1_get_len(unsigned char **p,
                         const unsigned char *end,
                         size_t *len, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Get the tag and length of the element.
 *              Check for the requested tag.
 *              Updates the pointer to immediately behind the tag and length.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              after the length, i.e. the first byte of the content.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param len   On successful completion, \c *len contains the length
 *              read from the ASN.1 input.
 * \param tag   The expected tag.
 *
 * \return      0 if successful.
 * \return      #MBEDTLS_ERR_ASN1_UNEXPECTED_TAG if the data does not start
 *              with the requested tag.
 * \return      #MBEDTLS_ERR_ASN1_OUT_OF_DATA if the ASN.1 element
 *              would end beyond \p end.
 * \return      #MBEDTLS_ERR_ASN1_INVALID_LENGTH if the length is unparsable.
 */
int mbedtls_asn1_get_tag(unsigned char **p,
                         const unsigned char *end,
                         size_t *len, int tag, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve a boolean ASN.1 tag and its value.
 *              Updates the pointer to immediately behind the full tag.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              beyond the ASN.1 element.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param val   On success, the parsed value (\c 0 or \c 1).
 *
 * \return      0 if successful.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 BOOLEAN.
 */
int mbedtls_asn1_get_bool(unsigned char **p,
                          const unsigned char *end,
                          int *val, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve an integer ASN.1 tag and its value.
 *              Updates the pointer to immediately behind the full tag.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              beyond the ASN.1 element.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param val   On success, the parsed value.
 *
 * \return      0 if successful.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 INTEGER.
 * \return      #MBEDTLS_ERR_ASN1_INVALID_LENGTH if the parsed value does
 *              not fit in an \c int.
 */
int mbedtls_asn1_get_int(unsigned char **p,
                         const unsigned char *end,
                         int *val, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve a bitstring ASN.1 tag and its value.
 *              Updates the pointer to immediately behind the full tag.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p is equal to \p end.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param bs    On success, ::mbedtls_asn1_bitstring information about
 *              the parsed value.
 *
 * \return      0 if successful.
 * \return      #MBEDTLS_ERR_ASN1_LENGTH_MISMATCH if the input contains
 *              extra data after a valid BIT STRING.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 BIT STRING.
 */
int mbedtls_asn1_get_bitstring(unsigned char **p, const unsigned char *end,
                               mbedtls_asn1_bitstring *bs, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve a bitstring ASN.1 tag without unused bits and its
 *              value.
 *              Updates the pointer to the beginning of the bit/octet string.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              of the content of the BIT STRING.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param len   On success, \c *len is the length of the content in bytes.
 *
 * \return      0 if successful.
 * \return      #MBEDTLS_ERR_ASN1_INVALID_DATA if the input starts with
 *              a valid BIT STRING with a nonzero number of unused bits.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 BIT STRING.
 */
int mbedtls_asn1_get_bitstring_null(unsigned char **p,
                                    const unsigned char *end,
                                    size_t *len, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Parses and splits an ASN.1 "SEQUENCE OF <tag>".
 *              Updates the pointer to immediately behind the full sequence tag.
 *
 * This function allocates memory for the sequence elements. You can free
 * the allocated memory with mbedtls_asn1_sequence_free().
 *
 * \note        On error, this function may return a partial list in \p cur.
 *              You must set `cur->next = NULL` before calling this function!
 *              Otherwise it is impossible to distinguish a previously non-null
 *              pointer from a pointer to an object allocated by this function.
 *
 * \note        If the sequence is empty, this function does not modify
 *              \c *cur. If the sequence is valid and non-empty, this
 *              function sets `cur->buf.tag` to \p tag. This allows
 *              callers to distinguish between an empty sequence and
 *              a one-element sequence.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p is equal to \p end.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param cur   A ::mbedtls_asn1_sequence which this function fills.
 *              When this function returns, \c *cur is the head of a linked
 *              list. Each node in this list is allocated with
 *              mbedtls_calloc() apart from \p cur itself, and should
 *              therefore be freed with mbedtls_free().
 *              The list describes the content of the sequence.
 *              The head of the list (i.e. \c *cur itself) describes the
 *              first element, `*cur->next` describes the second element, etc.
 *              For each element, `buf.tag == tag`, `buf.len` is the length
 *              of the content of the content of the element, and `buf.p`
 *              points to the first byte of the content (i.e. immediately
 *              past the length of the element).
 *              Note that list elements may be allocated even on error.
 * \param tag   Each element of the sequence must have this tag.
 *
 * \return      0 if successful.
 * \return      #MBEDTLS_ERR_ASN1_LENGTH_MISMATCH if the input contains
 *              extra data after a valid SEQUENCE OF \p tag.
 * \return      #MBEDTLS_ERR_ASN1_UNEXPECTED_TAG if the input starts with
 *              an ASN.1 SEQUENCE in which an element has a tag that
 *              is different from \p tag.
 * \return      #MBEDTLS_ERR_ASN1_ALLOC_FAILED if a memory allocation failed.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 SEQUENCE.
 */
int mbedtls_asn1_get_sequence_of(unsigned char **p,
                                 const unsigned char *end,
                                 mbedtls_asn1_sequence *cur,
                                 int tag, MinTlsDiagnostics* diagnostics);
/**
 * \brief          Free a heap-allocated linked list presentation of
 *                 an ASN.1 sequence, including the first element.
 *
 * There are two common ways to manage the memory used for the representation
 * of a parsed ASN.1 sequence:
 * - Allocate a head node `mbedtls_asn1_sequence *head` with mbedtls_calloc().
 *   Pass this node as the `cur` argument to mbedtls_asn1_get_sequence_of().
 *   When you have finished processing the sequence,
 *   call mbedtls_asn1_sequence_free() on `head`.
 * - Allocate a head node `mbedtls_asn1_sequence *head` in any manner,
 *   for example on the stack. Make sure that `head->next == NULL`.
 *   Pass `head` as the `cur` argument to mbedtls_asn1_get_sequence_of().
 *   When you have finished processing the sequence,
 *   call mbedtls_asn1_sequence_free() on `head->cur`,
 *   then free `head` itself in the appropriate manner.
 *
 * \param seq      The address of the first sequence component. This may
 *                 be \c NULL, in which case this functions returns
 *                 immediately.
 */
void mbedtls_asn1_sequence_free(mbedtls_asn1_sequence *seq);

/**
 * \brief                Traverse an ASN.1 SEQUENCE container and
 *                       call a callback for each entry.
 *
 * This function checks that the input is a SEQUENCE of elements that
 * each have a "must" tag, and calls a callback function on the elements
 * that have a "may" tag.
 *
 * For example, to validate that the input is a SEQUENCE of `tag1` and call
 * `cb` on each element, use
 * ```
 * mbedtls_asn1_traverse_sequence_of(&p, end, 0xff, tag1, 0, 0, cb, ctx);
 * ```
 *
 * To validate that the input is a SEQUENCE of ANY and call `cb` on
 * each element, use
 * ```
 * mbedtls_asn1_traverse_sequence_of(&p, end, 0, 0, 0, 0, cb, ctx);
 * ```
 *
 * To validate that the input is a SEQUENCE of CHOICE {NULL, OCTET STRING}
 * and call `cb` on each element that is an OCTET STRING, use
 * ```
 * mbedtls_asn1_traverse_sequence_of(&p, end, 0xfe, 0x04, 0xff, 0x04, cb, ctx);
 * ```
 *
 * The callback is called on the elements with a "may" tag from left to
 * right. If the input is not a valid SEQUENCE of elements with a "must" tag,
 * the callback is called on the elements up to the leftmost point where
 * the input is invalid.
 *
 * \warning              This function is still experimental and may change
 *                       at any time.
 *
 * \param p              The address of the pointer to the beginning of
 *                       the ASN.1 SEQUENCE header. This is updated to
 *                       point to the end of the ASN.1 SEQUENCE container
 *                       on a successful invocation.
 * \param end            The end of the ASN.1 SEQUENCE container.
 * \param tag_must_mask  A mask to be applied to the ASN.1 tags found within
 *                       the SEQUENCE before comparing to \p tag_must_val.
 * \param tag_must_val   The required value of each ASN.1 tag found in the
 *                       SEQUENCE, after masking with \p tag_must_mask.
 *                       Mismatching tags lead to an error.
 *                       For example, a value of \c 0 for both \p tag_must_mask
 *                       and \p tag_must_val means that every tag is allowed,
 *                       while a value of \c 0xFF for \p tag_must_mask means
 *                       that \p tag_must_val is the only allowed tag.
 * \param tag_may_mask   A mask to be applied to the ASN.1 tags found within
 *                       the SEQUENCE before comparing to \p tag_may_val.
 * \param tag_may_val    The desired value of each ASN.1 tag found in the
 *                       SEQUENCE, after masking with \p tag_may_mask.
 *                       Mismatching tags will be silently ignored.
 *                       For example, a value of \c 0 for \p tag_may_mask and
 *                       \p tag_may_val means that any tag will be considered,
 *                       while a value of \c 0xFF for \p tag_may_mask means
 *                       that all tags with value different from \p tag_may_val
 *                       will be ignored.
 * \param cb             The callback to trigger for each component
 *                       in the ASN.1 SEQUENCE that matches \p tag_may_val.
 *                       The callback function is called with the following
 *                       parameters:
 *                       - \p ctx.
 *                       - The tag of the current element.
 *                       - A pointer to the start of the current element's
 *                         content inside the input.
 *                       - The length of the content of the current element.
 *                       If the callback returns a non-zero value,
 *                       the function stops immediately,
 *                       forwarding the callback's return value.
 * \param ctx            The context to be passed to the callback \p cb.
 *
 * \return               \c 0 if successful the entire ASN.1 SEQUENCE
 *                       was traversed without parsing or callback errors.
 * \return               #MBEDTLS_ERR_ASN1_LENGTH_MISMATCH if the input
 *                       contains extra data after a valid SEQUENCE
 *                       of elements with an accepted tag.
 * \return               #MBEDTLS_ERR_ASN1_UNEXPECTED_TAG if the input starts
 *                       with an ASN.1 SEQUENCE in which an element has a tag
 *                       that is not accepted.
 * \return               An ASN.1 error code if the input does not start with
 *                       a valid ASN.1 SEQUENCE.
 * \return               A non-zero error code forwarded from the callback
 *                       \p cb in case the latter returns a non-zero value.
 */
int mbedtls_asn1_traverse_sequence_of(
    unsigned char **p,
    const unsigned char *end,
    unsigned char tag_must_mask, unsigned char tag_must_val,
    unsigned char tag_may_mask, unsigned char tag_may_val,
    int (*cb)(void *ctx, int tag,
              unsigned char *start, size_t len, MinTlsDiagnostics* diagnostics),
    void *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve an integer ASN.1 tag and its value.
 *              Updates the pointer to immediately behind the full tag.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              beyond the ASN.1 element.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param X     On success, the parsed value.
 *
 * \return      0 if successful.
 * \return      An ASN.1 error code if the input does not start with
 *              a valid ASN.1 INTEGER.
 * \return      #MBEDTLS_ERR_ASN1_INVALID_LENGTH if the parsed value does
 *              not fit in an \c int.
 * \return      An MPI error code if the parsed value is too large.
 */
int mbedtls_asn1_get_mpi(unsigned char **p,
                         const unsigned char *end,
                         mbedtls_mpi *X, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve an AlgorithmIdentifier ASN.1 sequence.
 *              Updates the pointer to immediately behind the full
 *              AlgorithmIdentifier.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              beyond the AlgorithmIdentifier element.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param alg   The buffer to receive the OID.
 * \param params The buffer to receive the parameters.
 *              This is zeroized if there are no parameters.
 *
 * \return      0 if successful or a specific ASN.1 or MPI error code.
 */
int mbedtls_asn1_get_alg(unsigned char **p,
                         const unsigned char *end,
                         mbedtls_asn1_buf *alg, mbedtls_asn1_buf *params, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Retrieve an AlgorithmIdentifier ASN.1 sequence with NULL or no
 *              params.
 *              Updates the pointer to immediately behind the full
 *              AlgorithmIdentifier.
 *
 * \param p     On entry, \c *p points to the start of the ASN.1 element.
 *              On successful completion, \c *p points to the first byte
 *              beyond the AlgorithmIdentifier element.
 *              On error, the value of \c *p is undefined.
 * \param end   End of data.
 * \param alg   The buffer to receive the OID.
 *
 * \return      0 if successful or a specific ASN.1 or MPI error code.
 */
int mbedtls_asn1_get_alg_null(unsigned char **p,
                              const unsigned char *end,
                              mbedtls_asn1_buf *alg, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

#endif

/**
 * \brief       Free all shallow entries in a mbedtls_asn1_named_data list,
 *              but do not free internal pointer targets.
 *
 * \param name  Head of the list of named data entries to free.
 *              This function calls mbedtls_free() on each list element.
 */
void mbedtls_asn1_free_named_data_list_shallow(mbedtls_asn1_named_data *name);

/** \} name Functions to parse ASN.1 data structures */
/** \} addtogroup asn1_module */

#ifdef __cplusplus
}
#endif

#endif /* asn1.h */

/* ===== Retained asn1write.h ===== */

/**
 * \file asn1write.h
 *
 * \brief ASN.1 buffer writing functionality
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/asn1write.h.
 */
#ifndef MBEDTLS_ASN1_WRITE_H
#define MBEDTLS_ASN1_WRITE_H



#define MBEDTLS_ASN1_CHK_ADD(g, f)                      \
    do                                                  \
    {                                                   \
        if ((ret = (f)) < 0)                         \
        MINTLS_RETURN(ret);                              \
        else                                            \
        (g) += ret;                                 \
    } while (0)

#define MBEDTLS_ASN1_CHK_CLEANUP_ADD(g, f)                      \
    do                                                  \
    {                                                   \
        if ((ret = (f)) < 0)                         \
        goto cleanup;                              \
        else                                            \
        (g) += ret;                                 \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           Write a length field in ASN.1 format.
 *
 * \note            This function works backwards in data buffer.
 *
 * \param p         The reference to the current position pointer.
 * \param start     The start of the buffer, for bounds-checking.
 * \param len       The length value to write.
 *
 * \return          The number of bytes written to \p p on success.
 * \return          A negative \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_asn1_write_len(unsigned char **p, const unsigned char *start,
                           size_t len, MinTlsDiagnostics* diagnostics);
/**
 * \brief           Write an ASN.1 tag in ASN.1 format.
 *
 * \note            This function works backwards in data buffer.
 *
 * \param p         The reference to the current position pointer.
 * \param start     The start of the buffer, for bounds-checking.
 * \param tag       The tag to write.
 *
 * \return          The number of bytes written to \p p on success.
 * \return          A negative \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_asn1_write_tag(unsigned char **p, const unsigned char *start,
                           unsigned char tag, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Write an arbitrary-precision number (#MBEDTLS_ASN1_INTEGER)
 *                  in ASN.1 format.
 *
 * \note            This function works backwards in data buffer.
 *
 * \param p         The reference to the current position pointer.
 * \param start     The start of the buffer, for bounds-checking.
 * \param X         The MPI to write.
 *                  It must be non-negative.
 *
 * \return          The number of bytes written to \p p on success.
 * \return          A negative \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_asn1_write_mpi(unsigned char **p, const unsigned char *start,
                           const mbedtls_mpi *X, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_ASN1_WRITE_H */

/* ===== Retained base64.h ===== */

/**
 * \file base64.h
 *
 * \brief RFC 1521 base64 encoding/decoding
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/base64.h.
 */
#ifndef MBEDTLS_BASE64_H
#define MBEDTLS_BASE64_H


#include <stddef.h>

/** Output buffer too small. */
#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL               -0x002A
/** Invalid character in input. */
#define MBEDTLS_ERR_BASE64_INVALID_CHARACTER              -0x002C

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Decode a base64-formatted buffer
 *
 * \param dst      destination buffer (can be NULL for checking size)
 * \param dlen     size of the destination buffer
 * \param olen     number of bytes written
 * \param src      source buffer
 * \param slen     amount of data to be decoded
 *
 * \return         0 if successful, MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL, or
 *                 MBEDTLS_ERR_BASE64_INVALID_CHARACTER if the input data is
 *                 not correct. *olen is always updated to reflect the amount
 *                 of data that has (or would have) been written.
 *
 * \note           Call this function with *dst = NULL or dlen = 0 to obtain
 *                 the required buffer size in *olen
 */
int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* base64.h */

/* ===== Retained base64_internal.h ===== */

/**
 * \file base64_internal.h
 *
 * \brief RFC 1521 base64 encoding/decoding: interfaces for invasive testing
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/base64_internal.h.
 */

#ifndef MBEDTLS_BASE64_INTERNAL
#define MBEDTLS_BASE64_INTERNAL


#endif /* MBEDTLS_BASE64_INTERNAL */

/* ===== Retained bignum_core.h ===== */

/**
 *  Core bignum functions
 *
 *  This interface should only be used by the legacy bignum module (bignum.h)
 *  and the modular bignum modules (bignum_mod.c, bignum_mod_raw.c). All other
 *  modules should use the high-level modular bignum interface (bignum_mod.h)
 *  or the legacy bignum interface (bignum.h). The only exceptions are:
 *  1. In ecp_curves.c, some mbedtls_ecp_mod_pXXX_raw() functions are using
 *  bignum_core functions. That's because those functions are implementing a
 *  part of bignum_mod: the optimized reduction in mbedtls_mpi_mod_modulus.
 *  2. In ecp.c, ecp_modp() is currently using bignum_core, while the rest of
 *  the module is using legacy bignum. That's transitional; eventually ecp.c is
 *  going to use bignum_mod_raw.
 *
 * This module is about processing non-negative integers with a fixed upper
 * bound that's of the form 2^n-1 where n is a multiple of #biL.
 * These can be thought of integers written in base 2^#biL with a fixed
 * number of digits. Digits in this base are called *limbs*.
 * Many operations treat these numbers as the principal representation of
 * a number modulo 2^n or a smaller bound.
 *
 * The functions in this module obey the following conventions unless
 * explicitly indicated otherwise:
 *
 * - **Overflow**: some functions indicate overflow from the range
 *   [0, 2^n-1] by returning carry parameters, while others operate
 *   modulo and so cannot overflow. This should be clear from the function
 *   documentation.
 * - **Bignum parameters**: Bignums are passed as pointers to an array of
 *   limbs. A limb has the type #mbedtls_mpi_uint. Unless otherwise specified:
 *     - Bignum parameters called \p A, \p B, ... are inputs, and are
 *       not modified by the function.
 *     - For operations modulo some number, the modulus is called \p N
 *       and is input-only.
 *     - Bignum parameters called \p X, \p Y are outputs or input-output.
 *       The initial content of output-only parameters is ignored.
 *     - Some functions use different names that reflect traditional
 *       naming of operands of certain operations (e.g.
 *       divisor/dividend/quotient/remainder).
 *     - \p T is a temporary storage area. The initial content of such
 *       parameter is ignored and the final content is unspecified.
 * - **Bignum sizes**: bignum sizes are always expressed in limbs.
 *   Most functions work on bignums of a given size and take a single
 *   \p limbs parameter that applies to all parameters that are limb arrays.
 *   All bignum sizes must be at least 1 and must be significantly less than
 *   #SIZE_MAX. The behavior if a size is 0 is undefined. The behavior if the
 *   total size of all parameters overflows #SIZE_MAX is undefined.
 * - **Parameter ordering**: for bignum parameters, outputs come before inputs.
 *   Temporaries come last.
 * - **Aliasing**: in general, output bignums may be aliased to one or more
 *   inputs. As an exception, parameters that are documented as a modulus value
 *   may not be aliased to an output. Outputs may not be aliased to one another.
 *   Temporaries may not be aliased to any other parameter.
 * - **Overlap**: apart from aliasing of limb array pointers (where two
 *   arguments are equal pointers), overlap is not supported and may result
 *   in undefined behavior.
 * - **Error handling**: This is a low-level module. Functions generally do not
 *   try to protect against invalid arguments such as nonsensical sizes or
 *   null pointers. Note that some functions that operate on bignums of
 *   different sizes have constraints about their size, and violating those
 *   constraints may lead to buffer overflows.
 * - **Modular representatives**: functions that operate modulo \p N expect
 *   all modular inputs to be in the range [0, \p N - 1] and guarantee outputs
 *   in the range [0, \p N - 1]. If an input is out of range, outputs are
 *   fully unspecified, though bignum values out of range should not cause
 *   buffer overflows (beware that this is not extensively tested).
 */

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/bignum_core.h.
 */

#ifndef MBEDTLS_BIGNUM_CORE_H
#define MBEDTLS_BIGNUM_CORE_H



/* ===== Retained constant_time_internal.h ===== */

/**
 *  Constant-time functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/constant_time_internal.h.
 */

#ifndef MBEDTLS_CONSTANT_TIME_INTERNAL_H
#define MBEDTLS_CONSTANT_TIME_INTERNAL_H

#include <stdint.h>
#include <stddef.h>



/* The constant-time interface provides various operations that are likely
 * to result in constant-time code that does not branch or use conditional
 * instructions for secret data (for secret pointers, this also applies to
 * the data pointed to).
 *
 * It has three main parts:
 *
 * - boolean operations
 *   These are all named mbedtls_ct_<type>_<operation>.
 *   They operate over <type> and return mbedtls_ct_condition_t.
 *   All arguments are considered secret.
 *   example: bool x = y | z          =>    x = mbedtls_ct_bool_or(y, z)
 *   example: bool x = y == z         =>    x = mbedtls_ct_uint_eq(y, z)
 *
 * - conditional data selection
 *   These are all named mbedtls_ct_<type>_if and mbedtls_ct_<type>_if_else_0
 *   All arguments are considered secret.
 *   example: size_t a = x ? b : c    =>    a = mbedtls_ct_size_if(x, b, c)
 *   example: unsigned a = x ? b : 0  =>    a = mbedtls_ct_uint_if_else_0(x, b)
 *
 * - block memory operations
 *   Only some arguments are considered secret, as documented for each
 *   function.
 *   example: if (x) memcpy(...)      =>    mbedtls_ct_memcpy_if(x, ...)
 *
 * mbedtls_ct_condition_t must be treated as opaque and only created and
 * manipulated via the functions in this header. The compiler should never
 * be able to prove anything about its value at compile-time.
 *
 * mbedtls_ct_uint_t is an unsigned integer type over which constant time
 * operations may be performed via the functions in this header. It is as big
 * as the larger of size_t and mbedtls_mpi_uint, i.e. it is safe to cast
 * to/from "unsigned int", "size_t", and "mbedtls_mpi_uint" (and any other
 * not-larger integer types).
 *
 * For Arm (32-bit, 64-bit and Thumb), x86 and x86-64, assembly implementations
 * are used to ensure that the generated code is constant time. For other
 * architectures, it uses a plain C fallback designed to yield constant-time code
 * (this has been observed to be constant-time on latest gcc, clang and MSVC
 * as of May 2023).
 *
 * For readability, the static inline definitions are separated out into
 * constant_time_impl.h.
 */

#if (SIZE_MAX > 0xffffffffffffffffULL)
/* Pointer size > 64-bit */
typedef size_t    mbedtls_ct_condition_t;
typedef size_t    mbedtls_ct_uint_t;
typedef ptrdiff_t mbedtls_ct_int_t;
#define MBEDTLS_CT_TRUE  ((mbedtls_ct_condition_t) mbedtls_ct_compiler_opaque(SIZE_MAX))
#elif (SIZE_MAX > 0xffffffff) || defined(MBEDTLS_HAVE_INT64)
/* 32-bit < pointer size <= 64-bit, or 64-bit MPI */
typedef uint64_t  mbedtls_ct_condition_t;
typedef uint64_t  mbedtls_ct_uint_t;
typedef int64_t   mbedtls_ct_int_t;
#define MBEDTLS_CT_SIZE_64
#define MBEDTLS_CT_TRUE  ((mbedtls_ct_condition_t) mbedtls_ct_compiler_opaque(UINT64_MAX))
#else
/* Pointer size <= 32-bit, and no 64-bit MPIs */
typedef uint32_t  mbedtls_ct_condition_t;
typedef uint32_t  mbedtls_ct_uint_t;
typedef int32_t   mbedtls_ct_int_t;
#define MBEDTLS_CT_SIZE_32
#define MBEDTLS_CT_TRUE  ((mbedtls_ct_condition_t) mbedtls_ct_compiler_opaque(UINT32_MAX))
#endif
#define MBEDTLS_CT_FALSE ((mbedtls_ct_condition_t) mbedtls_ct_compiler_opaque(0))

/* ============================================================================
 * Boolean operations
 */

/** Convert a number into a mbedtls_ct_condition_t.
 *
 * \param x Number to convert.
 *
 * \return MBEDTLS_CT_TRUE if \p x != 0, or MBEDTLS_CT_FALSE if \p x == 0
 *
 */
static inline mbedtls_ct_condition_t mbedtls_ct_bool(mbedtls_ct_uint_t x);

/** Boolean "not equal" operation.
 *
 * Functionally equivalent to:
 *
 * \p x != \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x != \p y, otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_uint_ne(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y);

/** Boolean "equals" operation.
 *
 * Functionally equivalent to:
 *
 * \p x == \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x == \p y, otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_uint_eq(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y);

/** Boolean "less than" operation.
 *
 * Functionally equivalent to:
 *
 * \p x < \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x < \p y, otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_uint_lt(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y);

/** Boolean "greater than" operation.
 *
 * Functionally equivalent to:
 *
 * \p x > \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x > \p y, otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_uint_gt(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y);

/** Boolean "greater or equal" operation.
 *
 * Functionally equivalent to:
 *
 * \p x >= \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x >= \p y,
 *              otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_uint_ge(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y);

/** Boolean "and" operation.
 *
 * Functionally equivalent to:
 *
 * \p x && \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x && \p y,
 *              otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_bool_and(mbedtls_ct_condition_t x,
                                                         mbedtls_ct_condition_t y);

/** Boolean "or" operation.
 *
 * Functionally equivalent to:
 *
 * \p x || \p y
 *
 * \param x     The first value to analyze.
 * \param y     The second value to analyze.
 *
 * \return      MBEDTLS_CT_TRUE if \p x || \p y,
 *              otherwise MBEDTLS_CT_FALSE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_bool_or(mbedtls_ct_condition_t x,
                                                        mbedtls_ct_condition_t y);

/** Boolean "not" operation.
 *
 * Functionally equivalent to:
 *
 * ! \p x
 *
 * \param x     The value to invert
 *
 * \return      MBEDTLS_CT_FALSE if \p x, otherwise MBEDTLS_CT_TRUE.
 */
static inline mbedtls_ct_condition_t mbedtls_ct_bool_not(mbedtls_ct_condition_t x);

/* ============================================================================
 * Data selection operations
 */

/** Choose between two size_t values.
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : if0.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 * \param if0           Value to use if \p condition == MBEDTLS_CT_FALSE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise \c if0.
 */
static inline size_t mbedtls_ct_size_if(mbedtls_ct_condition_t condition,
                                        size_t if1,
                                        size_t if0);

/** Choose between two unsigned values.
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : if0.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 * \param if0           Value to use if \p condition == MBEDTLS_CT_FALSE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise \c if0.
 */
static inline unsigned mbedtls_ct_uint_if(mbedtls_ct_condition_t condition,
                                          unsigned if1,
                                          unsigned if0);

/** Choose between two mbedtls_mpi_uint values.
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : if0.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 * \param if0           Value to use if \p condition == MBEDTLS_CT_FALSE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise \c if0.
 */
static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if(mbedtls_ct_condition_t condition, \
                                                      mbedtls_mpi_uint if1, \
                                                      mbedtls_mpi_uint if0);

/** Choose between an unsigned value and 0.
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : 0.
 *
 * Functionally equivalent to mbedtls_ct_uint_if(condition, if1, 0) but
 * results in smaller code size.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise 0.
 */
static inline unsigned mbedtls_ct_uint_if_else_0(mbedtls_ct_condition_t condition, unsigned if1);

/** Choose between an mbedtls_mpi_uint value and 0.
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : 0.
 *
 * Functionally equivalent to mbedtls_ct_mpi_uint_if(condition, if1, 0) but
 * results in smaller code size.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise 0.
 */
static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if_else_0(mbedtls_ct_condition_t condition,
                                                             mbedtls_mpi_uint if1);

/** Constant-flow char selection
 *
 * \param low   Secret. Bottom of range
 * \param high  Secret. Top of range
 * \param c     Secret. Value to compare to range
 * \param t     Secret. Value to return, if in range
 *
 * \return      \p t if \p low <= \p c <= \p high, 0 otherwise.
 */
static inline unsigned char mbedtls_ct_uchar_in_range_if(unsigned char low,
                                                         unsigned char high,
                                                         unsigned char c,
                                                         unsigned char t);

/** Choose between two error values. The values must be in the range [-32767..0].
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : if0.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 * \param if0           Value to use if \p condition == MBEDTLS_CT_FALSE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise \c if0.
 */
static inline int mbedtls_ct_error_if(mbedtls_ct_condition_t condition, int if1, int if0);

/** Choose between an error value and 0. The error value must be in the range [-32767..0].
 *
 * Functionally equivalent to:
 *
 * condition ? if1 : 0.
 *
 * Functionally equivalent to mbedtls_ct_error_if(condition, if1, 0) but
 * results in smaller code size.
 *
 * \param condition     Condition to test.
 * \param if1           Value to use if \p condition == MBEDTLS_CT_TRUE.
 *
 * \return  \c if1 if \p condition == MBEDTLS_CT_TRUE, otherwise 0.
 */
static inline int mbedtls_ct_error_if_else_0(mbedtls_ct_condition_t condition, int if1);

/* ============================================================================
 * Block memory operations
 */

/** Conditionally set a block of memory to zero.
 *
 * Regardless of the condition, every byte will be read once and written to
 * once.
 *
 * \param condition     Secret. Condition to test.
 * \param buf           Secret. Pointer to the start of the buffer.
 * \param len           Number of bytes to set to zero.
 *
 * \warning Unlike mbedtls_platform_zeroize, this does not have the same guarantees
 * about not being optimised away if the memory is never read again.
 */
void mbedtls_ct_zeroize_if(mbedtls_ct_condition_t condition, void *buf, size_t len);

/** Shift some data towards the left inside a buffer.
 *
 * Functionally equivalent to:
 *
 * memmove(start, start + offset, total - offset);
 * memset(start + (total - offset), 0, offset);
 *
 * Timing independence comes at the expense of performance.
 *
 * \param start     Secret. Pointer to the start of the buffer.
 * \param total     Total size of the buffer.
 * \param offset    Secret. Offset from which to copy \p total - \p offset bytes.
 */
void mbedtls_ct_memmove_left(void *start,
                             size_t total,
                             size_t offset);

/** Conditional memcpy.
 *
 * Functionally equivalent to:
 *
 * if (condition) {
 *      memcpy(dest, src1, len);
 * } else {
 *      if (src2 != NULL)
 *          memcpy(dest, src2, len);
 * }
 *
 * It will always read len bytes from src1.
 * If src2 != NULL, it will always read len bytes from src2.
 * If src2 == NULL, it will instead read len bytes from dest (as if src2 == dest).
 *
 * \param condition The condition
 * \param dest      Secret. Destination pointer.
 * \param src1      Secret. Pointer to copy from (if \p condition == MBEDTLS_CT_TRUE).
 *                  This may be equal to \p dest, but may not overlap in other ways.
 * \param src2      Secret (contents only - may branch to determine if this parameter is NULL).
 *                  Pointer to copy from (if \p condition == MBEDTLS_CT_FALSE and \p src2 is not NULL). May be NULL.
 *                  This may be equal to \p dest, but may not overlap it in other ways. It may overlap with \p src1.
 * \param len       Number of bytes to copy.
 */
void mbedtls_ct_memcpy_if(mbedtls_ct_condition_t condition,
                          unsigned char *dest,
                          const unsigned char *src1,
                          const unsigned char *src2,
                          size_t len
                          );

/* Documented in include/mbedtls/constant_time.h. a and b are secret.

   int mbedtls_ct_memcmp(const void *a,
                         const void *b,
                         size_t n);
 */

/* Include the implementation of static inline functions above. */
/* ===== Retained constant_time_impl.h ===== */

/**
 *  Constant-time functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/constant_time_impl.h.
 */

#ifndef MBEDTLS_CONSTANT_TIME_IMPL_H
#define MBEDTLS_CONSTANT_TIME_IMPL_H

#include <stddef.h>



/*
 * To improve readability of constant_time_internal.h, the static inline
 * definitions are here, and constant_time_internal.h has only the declarations.
 *
 * This results in duplicate declarations of the form:
 *     static inline void f();         // from constant_time_internal.h
 *     static inline void f() { ... }  // from constant_time_impl.h
 * when constant_time_internal.h is included.
 *
 * This appears to behave as if the declaration-without-definition was not present
 * (except for warnings if gcc -Wredundant-decls or similar is used).
 *
 * Disable -Wredundant-decls so that gcc does not warn about this. This is re-enabled
 * at the bottom of this file.
 */
#if defined(MBEDTLS_COMPILER_IS_GCC) && (__GNUC__ > 4)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

/* armcc5 --gnu defines __GNUC__ but doesn't support GNU's extended asm */
#if defined(MBEDTLS_HAVE_ASM) && defined(__GNUC__) && (!defined(__ARMCC_VERSION) || \
    __ARMCC_VERSION >= 6000000)
#define MBEDTLS_CT_ASM
#if (defined(__arm__) || defined(__thumb__) || defined(__thumb2__))
#define MBEDTLS_CT_ARM_ASM
#elif defined(__aarch64__)
#define MBEDTLS_CT_AARCH64_ASM
#elif defined(__amd64__) || defined(__x86_64__)
#define MBEDTLS_CT_X86_64_ASM
#elif defined(__i386__)
#define MBEDTLS_CT_X86_ASM
#endif
#endif

#define MBEDTLS_CT_SIZE (sizeof(mbedtls_ct_uint_t) * 8)

/* ============================================================================
 * Core const-time primitives
 */

/* Ensure that the compiler cannot know the value of x (i.e., cannot optimise
 * based on its value) after this function is called.
 *
 * If we are not using assembly, this will be fairly inefficient, so its use
 * should be minimised.
 */

#if !defined(MBEDTLS_CT_ASM)
extern volatile mbedtls_ct_uint_t mbedtls_ct_zero;
#endif

/**
 * \brief   Ensure that a value cannot be known at compile time.
 *
 * \param x        The value to hide from the compiler.
 * \return         The same value that was passed in, such that the compiler
 *                 cannot prove its value (even for calls of the form
 *                 x = mbedtls_ct_compiler_opaque(1), x will be unknown).
 *
 * \note           This is mainly used in constructing mbedtls_ct_condition_t
 *                 values and performing operations over them, to ensure that
 *                 there is no way for the compiler to ever know anything about
 *                 the value of an mbedtls_ct_condition_t.
 */
static inline mbedtls_ct_uint_t mbedtls_ct_compiler_opaque(mbedtls_ct_uint_t x)
{
#if defined(MBEDTLS_CT_ASM)
    asm volatile ("" : [x] "+r" (x) :);
    return x;
#else
    return x ^ mbedtls_ct_zero;
#endif
}

/*
 * Selecting unified syntax is needed for gcc, and harmless on clang.
 *
 * This is needed because on Thumb 1, condition flags are always set, so
 * e.g. "negs" is supported but "neg" is not (on Thumb 2, both exist).
 *
 * Under Thumb 1 unified syntax, only the "negs" form is accepted, and
 * under divided syntax, only the "neg" form is accepted. clang only
 * supports unified syntax.
 *
 * On Thumb 2 and Arm, both compilers are happy with the "s" suffix,
 * although we don't actually care about setting the flags.
 *
 * For old versions of gcc (see #8516 for details), restore divided
 * syntax afterwards - otherwise old versions of gcc seem to apply
 * unified syntax globally, which breaks other asm code.
 */
#if defined(MBEDTLS_COMPILER_IS_GCC) && defined(__thumb__) && !defined(__thumb2__) && \
    (__GNUC__ < 11) && !defined(__ARM_ARCH_2__)
#define RESTORE_ASM_SYNTAX  ".syntax divided                      \n\t"
#else
#define RESTORE_ASM_SYNTAX
#endif

/* Convert a number into a condition in constant time. */
static inline mbedtls_ct_condition_t mbedtls_ct_bool(mbedtls_ct_uint_t x)
{
    /*
     * Define mask-generation code that, as far as possible, will not use branches or conditional instructions.
     *
     * For some platforms / type sizes, we define assembly to assure this.
     *
     * Otherwise, we define a plain C fallback which (in May 2023) does not get optimised into
     * conditional instructions or branches by trunk clang, gcc, or MSVC v19.
     */
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    mbedtls_ct_uint_t s;
    asm volatile ("neg %x[s], %x[x]                               \n\t"
                  "orr %x[x], %x[s], %x[x]                        \n\t"
                  "asr %x[x], %x[x], 63                           \n\t"
                  :
                  [s] "=&r" (s),
                  [x] "+&r" (x)
                  :
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s;
    asm volatile (".syntax unified                                \n\t"
                  "negs %[s], %[x]                                \n\t"
                  "orrs %[x], %[x], %[s]                          \n\t"
                  "asrs %[x], %[x], #31                           \n\t"
                  RESTORE_ASM_SYNTAX
                  :
                  [s] "=&l" (s),
                  [x] "+&l" (x)
                  :
                  :
                  "cc" /* clobbers flag bits */
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_X86_64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    uint64_t s;
    asm volatile ("mov  %[x], %[s]                                \n\t"
                  "neg  %[s]                                      \n\t"
                  "or   %[x], %[s]                                \n\t"
                  "sar  $63, %[s]                                 \n\t"
                  :
                  [s] "=&a" (s)
                  :
                  [x] "D" (x)
                  :
                  );
    return (mbedtls_ct_condition_t) s;
#elif defined(MBEDTLS_CT_X86_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s;
    asm volatile ("mov %[x], %[s]                                 \n\t"
                  "neg %[s]                                       \n\t"
                  "or %[s], %[x]                                  \n\t"
                  "sar $31, %[x]                                  \n\t"
                  :
                  [s] "=&c" (s),
                  [x] "+&a" (x)
                  :
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#else
    const mbedtls_ct_uint_t xo = mbedtls_ct_compiler_opaque(x);
#if defined(_MSC_VER)
    /* MSVC has a warning about unary minus on unsigned, but this is
     * well-defined and precisely what we want to do here */
#pragma warning( push )
#pragma warning( disable : 4146 )
#endif
    // y is negative (i.e., top bit set) iff x is non-zero
    mbedtls_ct_int_t y = (-xo) | -(xo >> 1);

    // extract only the sign bit of y so that y == 1 (if x is non-zero) or 0 (if x is zero)
    y = (((mbedtls_ct_uint_t) y) >> (MBEDTLS_CT_SIZE - 1));

    // -y has all bits set (if x is non-zero), or all bits clear (if x is zero)
    return (mbedtls_ct_condition_t) (-y);
#if defined(_MSC_VER)
#pragma warning( pop )
#endif
#endif
}

static inline mbedtls_ct_uint_t mbedtls_ct_if(mbedtls_ct_condition_t condition,
                                              mbedtls_ct_uint_t if1,
                                              mbedtls_ct_uint_t if0)
{
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    asm volatile ("and %x[if1], %x[if1], %x[condition]            \n\t"
                  "mvn %x[condition], %x[condition]               \n\t"
                  "and %x[condition], %x[condition], %x[if0]      \n\t"
                  "orr %x[condition], %x[if1], %x[condition]"
                  :
                  [condition] "+&r" (condition),
                  [if1] "+&r" (if1)
                  :
                  [if0] "r" (if0)
                  :
                  );
    return (mbedtls_ct_uint_t) condition;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    asm volatile (".syntax unified                                \n\t"
                  "ands %[if1], %[if1], %[condition]              \n\t"
                  "mvns %[condition], %[condition]                \n\t"
                  "ands %[condition], %[condition], %[if0]        \n\t"
                  "orrs %[condition], %[if1], %[condition]        \n\t"
                  RESTORE_ASM_SYNTAX
                  :
                  [condition] "+&l" (condition),
                  [if1] "+&l" (if1)
                  :
                  [if0] "l" (if0)
                  :
                  "cc"
                  );
    return (mbedtls_ct_uint_t) condition;
#elif defined(MBEDTLS_CT_X86_64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    asm volatile ("and  %[condition], %[if1]                      \n\t"
                  "not  %[condition]                              \n\t"
                  "and  %[condition], %[if0]                      \n\t"
                  "or   %[if1], %[if0]                            \n\t"
                  :
                  [condition] "+&D" (condition),
                  [if1] "+&S" (if1),
                  [if0] "+&a" (if0)
                  :
                  :
                  );
    return if0;
#elif defined(MBEDTLS_CT_X86_ASM) && defined(MBEDTLS_CT_SIZE_32)
    asm volatile ("and %[condition], %[if1]                       \n\t"
                  "not %[condition]                               \n\t"
                  "and %[if0], %[condition]                       \n\t"
                  "or %[condition], %[if1]                        \n\t"
                  :
                  [condition] "+&c" (condition),
                  [if1] "+&a" (if1)
                  :
                  [if0] "b" (if0)
                  :
                  );
    return if1;
#else
    mbedtls_ct_condition_t not_cond =
        (mbedtls_ct_condition_t) (~mbedtls_ct_compiler_opaque(condition));
    return (mbedtls_ct_uint_t) ((condition & if1) | (not_cond & if0));
#endif
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_lt(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y)
{
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    uint64_t s1;
    asm volatile ("eor     %x[s1], %x[y], %x[x]                   \n\t"
                  "sub     %x[x], %x[x], %x[y]                    \n\t"
                  "bic     %x[x], %x[x], %x[s1]                   \n\t"
                  "and     %x[s1], %x[s1], %x[y]                  \n\t"
                  "orr     %x[s1], %x[x], %x[s1]                  \n\t"
                  "asr     %x[x], %x[s1], 63"
                  :
                  [s1] "=&r" (s1),
                  [x] "+&r" (x)
                  :
                  [y] "r" (y)
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s1;
    asm volatile (
        ".syntax unified                                          \n\t"
#if defined(__thumb__) && !defined(__thumb2__)
        "movs     %[s1], %[x]                                     \n\t"
        "eors     %[s1], %[s1], %[y]                              \n\t"
#else
        "eors     %[s1], %[x], %[y]                               \n\t"
#endif
        "subs    %[x], %[x], %[y]                                 \n\t"
        "bics    %[x], %[x], %[s1]                                \n\t"
        "ands    %[y], %[s1], %[y]                                \n\t"
        "orrs    %[x], %[x], %[y]                                 \n\t"
        "asrs    %[x], %[x], #31                                  \n\t"
        RESTORE_ASM_SYNTAX
        :
        [s1] "=&l" (s1),
        [x] "+&l" (x),
        [y] "+&l" (y)
        :
        :
        "cc"
        );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_X86_64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    uint64_t s;
    asm volatile ("mov %[x], %[s]                                 \n\t"
                  "xor %[y], %[s]                                 \n\t"
                  "sub %[y], %[x]                                 \n\t"
                  "and %[s], %[y]                                 \n\t"
                  "not %[s]                                       \n\t"
                  "and %[s], %[x]                                 \n\t"
                  "or %[y], %[x]                                  \n\t"
                  "sar $63, %[x]                                  \n\t"
                  :
                  [s] "=&a" (s),
                  [x] "+&D" (x),
                  [y] "+&S" (y)
                  :
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_X86_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s;
    asm volatile ("mov %[x], %[s]                                 \n\t"
                  "xor %[y], %[s]                                 \n\t"
                  "sub %[y], %[x]                                 \n\t"
                  "and %[s], %[y]                                 \n\t"
                  "not %[s]                                       \n\t"
                  "and %[s], %[x]                                 \n\t"
                  "or  %[y], %[x]                                 \n\t"
                  "sar $31, %[x]                                  \n\t"
                  :
                  [s] "=&b" (s),
                  [x] "+&a" (x),
                  [y] "+&c" (y)
                  :
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#else
    /* Ensure that the compiler cannot optimise the following operations over x and y,
     * even if it knows the value of x and y.
     */
    const mbedtls_ct_uint_t xo = mbedtls_ct_compiler_opaque(x);
    const mbedtls_ct_uint_t yo = mbedtls_ct_compiler_opaque(y);
    /*
     * Check if the most significant bits (MSB) of the operands are different.
     * cond is true iff the MSBs differ.
     */
    mbedtls_ct_condition_t cond = mbedtls_ct_bool((xo ^ yo) >> (MBEDTLS_CT_SIZE - 1));

    /*
     * If the MSB are the same then the difference x-y will be negative (and
     * have its MSB set to 1 during conversion to unsigned) if and only if x<y.
     *
     * If the MSB are different, then the operand with the MSB of 1 is the
     * bigger. (That is if y has MSB of 1, then x<y is true and it is false if
     * the MSB of y is 0.)
     */

    // Select either y, or x - y
    mbedtls_ct_uint_t ret = mbedtls_ct_if(cond, yo, (mbedtls_ct_uint_t) (xo - yo));

    // Extract only the MSB of ret
    ret = ret >> (MBEDTLS_CT_SIZE - 1);

    // Convert to a condition (i.e., all bits set iff non-zero)
    return mbedtls_ct_bool(ret);
#endif
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_ne(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y)
{
    /* diff = 0 if x == y, non-zero otherwise */
    const mbedtls_ct_uint_t diff = mbedtls_ct_compiler_opaque(x) ^ mbedtls_ct_compiler_opaque(y);

    /* all ones if x != y, 0 otherwise */
    return mbedtls_ct_bool(diff);
}

static inline unsigned char mbedtls_ct_uchar_in_range_if(unsigned char low,
                                                         unsigned char high,
                                                         unsigned char c,
                                                         unsigned char t)
{
    const unsigned char co = (unsigned char) mbedtls_ct_compiler_opaque(c);
    const unsigned char to = (unsigned char) mbedtls_ct_compiler_opaque(t);

    /* low_mask is: 0 if low <= c, 0x...ff if low > c */
    unsigned low_mask = ((unsigned) co - low) >> 8;
    /* high_mask is: 0 if c <= high, 0x...ff if c > high */
    unsigned high_mask = ((unsigned) high - co) >> 8;

    return (unsigned char) (~(low_mask | high_mask)) & to;
}

/* ============================================================================
 * Everything below here is trivial wrapper functions
 */

static inline size_t mbedtls_ct_size_if(mbedtls_ct_condition_t condition,
                                        size_t if1,
                                        size_t if0)
{
    return (size_t) mbedtls_ct_if(condition, (mbedtls_ct_uint_t) if1, (mbedtls_ct_uint_t) if0);
}

static inline unsigned mbedtls_ct_uint_if(mbedtls_ct_condition_t condition,
                                          unsigned if1,
                                          unsigned if0)
{
    return (unsigned) mbedtls_ct_if(condition, (mbedtls_ct_uint_t) if1, (mbedtls_ct_uint_t) if0);
}

static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if(mbedtls_ct_condition_t condition,
                                                      mbedtls_mpi_uint if1,
                                                      mbedtls_mpi_uint if0)
{
    return (mbedtls_mpi_uint) mbedtls_ct_if(condition,
                                            (mbedtls_ct_uint_t) if1,
                                            (mbedtls_ct_uint_t) if0);
}

static inline unsigned mbedtls_ct_uint_if_else_0(mbedtls_ct_condition_t condition, unsigned if1)
{
    return (unsigned) (condition & if1);
}

static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if_else_0(mbedtls_ct_condition_t condition,
                                                             mbedtls_mpi_uint if1)
{
    return (mbedtls_mpi_uint) (condition & if1);
}

static inline int mbedtls_ct_error_if(mbedtls_ct_condition_t condition, int if1, int if0)
{
    /* Coverting int -> uint -> int here is safe, because we require if1 and if0 to be
     * in the range -32767..0, and we require 32-bit int and uint types.
     *
     * This means that (0 <= -if0 < INT_MAX), so negating if0 is safe, and similarly for
     * converting back to int.
     */
    return -((int) mbedtls_ct_if(condition, (mbedtls_ct_uint_t) (-if1),
                                 (mbedtls_ct_uint_t) (-if0)));
}

static inline int mbedtls_ct_error_if_else_0(mbedtls_ct_condition_t condition, int if1)
{
    return -((int) (condition & (-if1)));
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_eq(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return ~mbedtls_ct_uint_ne(x, y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_gt(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return mbedtls_ct_uint_lt(y, x);
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_ge(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return ~mbedtls_ct_uint_lt(x, y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_and(mbedtls_ct_condition_t x,
                                                         mbedtls_ct_condition_t y)
{
    return (mbedtls_ct_condition_t) (x & y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_or(mbedtls_ct_condition_t x,
                                                        mbedtls_ct_condition_t y)
{
    return (mbedtls_ct_condition_t) (x | y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_not(mbedtls_ct_condition_t x)
{
    return (mbedtls_ct_condition_t) (~x);
}

#if defined(MBEDTLS_COMPILER_IS_GCC) && (__GNUC__ > 4)
/* Restore warnings for -Wredundant-decls on gcc */
    #pragma GCC diagnostic pop
#endif

#endif /* MBEDTLS_CONSTANT_TIME_IMPL_H */


#endif /* MBEDTLS_CONSTANT_TIME_INTERNAL_H */


#define ciL    (sizeof(mbedtls_mpi_uint))     /** chars in limb  */
#define biL    (ciL << 3)                     /** bits  in limb  */
#define biH    (ciL << 2)                     /** half limb size */

/*
 * Convert between bits/chars and number of limbs
 * Divide first in order to avoid potential overflows
 */
#define BITS_TO_LIMBS(i)  ((i) / biL + ((i) % biL != 0))
#define CHARS_TO_LIMBS(i) ((i) / ciL + ((i) % ciL != 0))
/* Get a specific byte, without range checks. */
#define GET_BYTE(X, i)                                \
    (((X)[(i) / ciL] >> (((i) % ciL) * 8)) & 0xff)

/* Constants to identify whether a value is public or secret. If a parameter is marked as secret by
 * this constant, the function must be constant time with respect to the parameter.
 *
 * This is only needed for functions with the _optionally_safe postfix. All other functions have
 * fixed behavior that can't be changed at runtime and are constant time with respect to their
 * parameters as prescribed by their documentation or by conventions in their module's documentation.
 *
 * Parameters should be named X_public where X is the name of the
 * corresponding input parameter.
 *
 * Implementation should always check using
 *  if (X_public == MBEDTLS_MPI_IS_PUBLIC) {
 *      // unsafe path
 *  } else {
 *      // safe path
 *  }
 * not the other way round, in order to prevent misuse. (That is, if a value
 * other than the two below is passed, default to the safe path.)
 *
 * The value of MBEDTLS_MPI_IS_PUBLIC is chosen in a way that is unlikely to happen by accident, but
 * which can be used as an immediate value in a Thumb2 comparison (for code size). */
#define MBEDTLS_MPI_IS_PUBLIC  0x2a2a2a2a
#define MBEDTLS_MPI_IS_SECRET  0

/** Count leading zero bits in a given integer.
 *
 * \warning     The result is undefined if \p a == 0
 *
 * \param a     Integer to count leading zero bits.
 *
 * \return      The number of leading zero bits in \p a, if \p a != 0.
 *              If \p a == 0, the result is undefined.
 */
size_t mbedtls_mpi_core_clz(mbedtls_mpi_uint a);

/** Return the minimum number of bits required to represent the value held
 * in the MPI.
 *
 * \note This function returns 0 if all the limbs of \p A are 0.
 *
 * \param[in] A     The address of the MPI.
 * \param A_limbs   The number of limbs of \p A.
 *
 * \return      The number of bits in \p A.
 */
size_t mbedtls_mpi_core_bitlen(const mbedtls_mpi_uint *A, size_t A_limbs);

/** Convert a big-endian byte array aligned to the size of mbedtls_mpi_uint
 * into the storage form used by mbedtls_mpi.
 *
 * \param[in,out] A     The address of the MPI.
 * \param A_limbs       The number of limbs of \p A.
 */
void mbedtls_mpi_core_bigendian_to_host(mbedtls_mpi_uint *A,
                                        size_t A_limbs);

/** \brief         Compare a machine integer with an MPI.
 *
 *                 This function operates in constant time with respect
 *                 to the values of \p min and \p A.
 *
 * \param min      A machine integer.
 * \param[in] A    An MPI.
 * \param A_limbs  The number of limbs of \p A.
 *                 This must be at least 1.
 *
 * \return         MBEDTLS_CT_TRUE if \p min is less than or equal to \p A, otherwise MBEDTLS_CT_FALSE.
 */
mbedtls_ct_condition_t mbedtls_mpi_core_uint_le_mpi(mbedtls_mpi_uint min,
                                                    const mbedtls_mpi_uint *A,
                                                    size_t A_limbs);

/**
 * \brief          Check if one unsigned MPI is less than another in constant
 *                 time.
 *
 * \param A        The left-hand MPI. This must point to an array of limbs
 *                 with the same allocated length as \p B.
 * \param B        The right-hand MPI. This must point to an array of limbs
 *                 with the same allocated length as \p A.
 * \param limbs    The number of limbs in \p A and \p B.
 *                 This must not be 0.
 *
 * \return         MBEDTLS_CT_TRUE  if \p A is less than \p B.
 *                 MBEDTLS_CT_FALSE if \p A is greater than or equal to \p B.
 */
mbedtls_ct_condition_t mbedtls_mpi_core_lt_ct(const mbedtls_mpi_uint *A,
                                              const mbedtls_mpi_uint *B,
                                              size_t limbs);

/**
 * \brief   Perform a safe conditional copy of an MPI which doesn't reveal
 *          whether assignment was done or not.
 *
 * \param[out] X        The address of the destination MPI.
 *                      This must be initialized. Must have enough limbs to
 *                      store the full value of \p A.
 * \param[in]  A        The address of the source MPI. This must be initialized.
 * \param      limbs    The number of limbs of \p A.
 * \param      assign   The condition deciding whether to perform the
 *                      assignment or not. Callers will need to use
 *                      the constant time interface (e.g. `mbedtls_ct_bool()`)
 *                      to construct this argument.
 *
 * \note           This function avoids leaking any information about whether
 *                 the assignment was done or not.
 */
void mbedtls_mpi_core_cond_assign(mbedtls_mpi_uint *X,
                                  const mbedtls_mpi_uint *A,
                                  size_t limbs,
                                  mbedtls_ct_condition_t assign);

/**
 * \brief   Perform a safe conditional swap of two MPIs which doesn't reveal
 *          whether the swap was done or not.
 *
 * \param[in,out] X         The address of the first MPI.
 *                          This must be initialized.
 * \param[in,out] Y         The address of the second MPI.
 *                          This must be initialized.
 * \param         limbs     The number of limbs of \p X and \p Y.
 * \param         swap      The condition deciding whether to perform
 *                          the swap or not.
 *
 * \note           This function avoids leaking any information about whether
 *                 the swap was done or not.
 */
void mbedtls_mpi_core_cond_swap(mbedtls_mpi_uint *X,
                                mbedtls_mpi_uint *Y,
                                size_t limbs,
                                mbedtls_ct_condition_t swap);

/** Import X from unsigned binary data, big-endian.
 *
 * The MPI needs to have enough limbs to store the full value (including any
 * most significant zero bytes in the input).
 *
 * \param[out] X        The address of the MPI.
 *                      May only be #NULL if \p X_limbs is 0 and \p input_length
 *                      is 0.
 * \param X_limbs       The number of limbs of \p X.
 * \param[in] input     The input buffer to import from.
 *                      May only be #NULL if \p input_length is 0.
 * \param input_length  The length in bytes of \p input.
 *
 * \return       \c 0 if successful.
 * \return       #MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL if \p X isn't
 *               large enough to hold the value in \p input.
 */
int mbedtls_mpi_core_read_be(mbedtls_mpi_uint *X,
                             size_t X_limbs,
                             const unsigned char *input,
                             size_t input_length, MinTlsDiagnostics* diagnostics);

/** Export A into unsigned binary data, little-endian.
 *
 * \note If \p output is shorter than \p A the export is still successful if the
 *       value held in \p A fits in the buffer (that is, if enough of the most
 *       significant bytes of \p A are 0).
 *
 * \param[in] A         The address of the MPI.
 * \param A_limbs       The number of limbs of \p A.
 * \param[out] output   The output buffer to export to.
 * \param output_length The length in bytes of \p output.
 *
 * \return       \c 0 if successful.
 * \return       #MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL if \p output isn't
 *               large enough to hold the value of \p A.
 */
int mbedtls_mpi_core_write_le(const mbedtls_mpi_uint *A,
                              size_t A_limbs,
                              unsigned char *output,
                              size_t output_length, MinTlsDiagnostics* diagnostics);

/** Export A into unsigned binary data, big-endian.
 *
 * \note If \p output is shorter than \p A the export is still successful if the
 *       value held in \p A fits in the buffer (that is, if enough of the most
 *       significant bytes of \p A are 0).
 *
 * \param[in] A         The address of the MPI.
 * \param A_limbs       The number of limbs of \p A.
 * \param[out] output   The output buffer to export to.
 * \param output_length The length in bytes of \p output.
 *
 * \return       \c 0 if successful.
 * \return       #MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL if \p output isn't
 *               large enough to hold the value of \p A.
 */
int mbedtls_mpi_core_write_be(const mbedtls_mpi_uint *A,
                              size_t A_limbs,
                              unsigned char *output,
                              size_t output_length, MinTlsDiagnostics* diagnostics);

/** \brief              Shift an MPI in-place right by a number of bits.
 *
 *                      Shifting by more bits than there are bit positions
 *                      in \p X is valid and results in setting \p X to 0.
 *
 *                      This function's execution time depends on the value
 *                      of \p count (and of course \p limbs).
 *
 * \param[in,out] X     The number to shift.
 * \param limbs         The number of limbs of \p X. This must be at least 1.
 * \param count         The number of bits to shift by.
 */
void mbedtls_mpi_core_shift_r(mbedtls_mpi_uint *X, size_t limbs,
                              size_t count);

/**
 * \brief               Shift an MPI in-place left by a number of bits.
 *
 *                      Shifting by more bits than there are bit positions
 *                      in \p X will produce an unspecified result.
 *
 *                      This function's execution time depends on the value
 *                      of \p count (and of course \p limbs).
 * \param[in,out] X     The number to shift.
 * \param limbs         The number of limbs of \p X. This must be at least 1.
 * \param count         The number of bits to shift by.
 */
void mbedtls_mpi_core_shift_l(mbedtls_mpi_uint *X, size_t limbs,
                              size_t count);

/**
 * \brief Add two fixed-size large unsigned integers, returning the carry.
 *
 * Calculates `A + B` where `A` and `B` have the same size.
 *
 * This function operates modulo `2^(biL*limbs)` and returns the carry
 * (1 if there was a wraparound, and 0 otherwise).
 *
 * \p X may be aliased to \p A or \p B.
 *
 * \param[out] X    The result of the addition.
 * \param[in] A     Little-endian presentation of the left operand.
 * \param[in] B     Little-endian presentation of the right operand.
 * \param limbs     Number of limbs of \p X, \p A and \p B.
 *
 * \return          1 if `A + B >= 2^(biL*limbs)`, 0 otherwise.
 */
mbedtls_mpi_uint mbedtls_mpi_core_add(mbedtls_mpi_uint *X,
                                      const mbedtls_mpi_uint *A,
                                      const mbedtls_mpi_uint *B,
                                      size_t limbs);

/**
 * \brief Conditional addition of two fixed-size large unsigned integers,
 *        returning the carry.
 *
 * Functionally equivalent to
 *
 * ```
 * if( cond )
 *    X += A;
 * return carry;
 * ```
 *
 * This function operates modulo `2^(biL*limbs)`.
 *
 * \param[in,out] X  The pointer to the (little-endian) array
 *                   representing the bignum to accumulate onto.
 * \param[in] A      The pointer to the (little-endian) array
 *                   representing the bignum to conditionally add
 *                   to \p X. This may be aliased to \p X but may not
 *                   overlap otherwise.
 * \param limbs      Number of limbs of \p X and \p A.
 * \param cond       Condition bit dictating whether addition should
 *                   happen or not. This must be \c 0 or \c 1.
 *
 * \warning          If \p cond is neither 0 nor 1, the result of this function
 *                   is unspecified, and the resulting value in \p X might be
 *                   neither its original value nor \p X + \p A.
 *
 * \return           1 if `X + cond * A >= 2^(biL*limbs)`, 0 otherwise.
 */
mbedtls_mpi_uint mbedtls_mpi_core_add_if(mbedtls_mpi_uint *X,
                                         const mbedtls_mpi_uint *A,
                                         size_t limbs,
                                         unsigned cond);

/**
 * \brief Subtract two fixed-size large unsigned integers, returning the borrow.
 *
 * Calculate `A - B` where \p A and \p B have the same size.
 * This function operates modulo `2^(biL*limbs)` and returns the carry
 * (1 if there was a wraparound, i.e. if `A < B`, and 0 otherwise).
 *
 * \p X may be aliased to \p A or \p B, or even both, but may not overlap
 * either otherwise.
 *
 * \param[out] X    The result of the subtraction.
 * \param[in] A     Little-endian presentation of left operand.
 * \param[in] B     Little-endian presentation of right operand.
 * \param limbs     Number of limbs of \p X, \p A and \p B.
 *
 * \return          1 if `A < B`.
 *                  0 if `A >= B`.
 */
mbedtls_mpi_uint mbedtls_mpi_core_sub(mbedtls_mpi_uint *X,
                                      const mbedtls_mpi_uint *A,
                                      const mbedtls_mpi_uint *B,
                                      size_t limbs);

/**
 * \brief Perform a fixed-size multiply accumulate operation: X += b * A
 *
 * \p X may be aliased to \p A (when \p X_limbs == \p A_limbs), but may not
 * otherwise overlap.
 *
 * This function operates modulo `2^(biL*X_limbs)`.
 *
 * \param[in,out] X  The pointer to the (little-endian) array
 *                   representing the bignum to accumulate onto.
 * \param X_limbs    The number of limbs of \p X. This must be
 *                   at least \p A_limbs.
 * \param[in] A      The pointer to the (little-endian) array
 *                   representing the bignum to multiply with.
 *                   This may be aliased to \p X but may not overlap
 *                   otherwise.
 * \param A_limbs    The number of limbs of \p A.
 * \param b          X scalar to multiply with.
 *
 * \return           The carry at the end of the operation.
 */
mbedtls_mpi_uint mbedtls_mpi_core_mla(mbedtls_mpi_uint *X, size_t X_limbs,
                                      const mbedtls_mpi_uint *A, size_t A_limbs,
                                      mbedtls_mpi_uint b);

/**
 * \brief Perform a known-size multiplication
 *
 * \p X may not be aliased to any of the inputs for this function.
 * \p A may be aliased to \p B.
 *
 * \param[out] X     The pointer to the (little-endian) array to receive
 *                   the product of \p A_limbs and \p B_limbs.
 *                   This must be of length \p A_limbs + \p B_limbs.
 * \param[in] A      The pointer to the (little-endian) array
 *                   representing the first factor.
 * \param A_limbs    The number of limbs in \p A.
 * \param[in] B      The pointer to the (little-endian) array
 *                   representing the second factor.
 * \param B_limbs    The number of limbs in \p B.
 */
void mbedtls_mpi_core_mul(mbedtls_mpi_uint *X,
                          const mbedtls_mpi_uint *A, size_t A_limbs,
                          const mbedtls_mpi_uint *B, size_t B_limbs);

/**
 * \brief Calculate initialisation value for fast Montgomery modular
 *        multiplication
 *
 * \param[in] N  Little-endian presentation of the modulus. This must have
 *               at least one limb.
 *
 * \return       The initialisation value for fast Montgomery modular multiplication
 */
mbedtls_mpi_uint mbedtls_mpi_core_montmul_init(const mbedtls_mpi_uint *N);

/**
 * \brief Montgomery multiplication: X = A * B * R^-1 mod N (HAC 14.36)
 *
 * \p A and \p B must be in canonical form. That is, < \p N.
 *
 * \p X may be aliased to \p A or \p N, or even \p B (if \p AN_limbs ==
 * \p B_limbs) but may not overlap any parameters otherwise.
 *
 * \p A and \p B may alias each other, if \p AN_limbs == \p B_limbs. They may
 * not alias \p N (since they must be in canonical form, they cannot == \p N).
 *
 * \param[out]    X         The destination MPI, as a little-endian array of
 *                          length \p AN_limbs.
 *                          On successful completion, X contains the result of
 *                          the multiplication `A * B * R^-1` mod N where
 *                          `R = 2^(biL*AN_limbs)`.
 * \param[in]     A         Little-endian presentation of first operand.
 *                          Must have the same number of limbs as \p N.
 * \param[in]     B         Little-endian presentation of second operand.
 * \param[in]     B_limbs   The number of limbs in \p B.
 *                          Must be <= \p AN_limbs.
 * \param[in]     N         Little-endian presentation of the modulus.
 *                          This must be odd, and have exactly the same number
 *                          of limbs as \p A.
 *                          It may alias \p X, but must not alias or otherwise
 *                          overlap any of the other parameters.
 * \param[in]     AN_limbs  The number of limbs in \p X, \p A and \p N.
 * \param         mm        The Montgomery constant for \p N: -N^-1 mod 2^biL.
 *                          This can be calculated by `mbedtls_mpi_core_montmul_init()`.
 * \param[in,out] T         Temporary storage of size at least 2*AN_limbs+1 limbs.
 *                          Its initial content is unused and
 *                          its final content is indeterminate.
 *                          It must not alias or otherwise overlap any of the
 *                          other parameters.
 */
void mbedtls_mpi_core_montmul(mbedtls_mpi_uint *X,
                              const mbedtls_mpi_uint *A,
                              const mbedtls_mpi_uint *B, size_t B_limbs,
                              const mbedtls_mpi_uint *N, size_t AN_limbs,
                              mbedtls_mpi_uint mm, mbedtls_mpi_uint *T);

/**
 * \brief Calculate the square of the Montgomery constant. (Needed
 *        for conversion and operations in Montgomery form.)
 *
 * \param[out] X  A pointer to the result of the calculation of
 *                the square of the Montgomery constant:
 *                2^{2*n*biL} mod N.
 * \param[in]  N  Little-endian presentation of the modulus, which must be odd.
 *
 * \return        0 if successful.
 * \return        #MBEDTLS_ERR_MPI_ALLOC_FAILED if there is not enough space
 *                to store the value of Montgomery constant squared.
 * \return        #MBEDTLS_ERR_MPI_DIVISION_BY_ZERO if \p N modulus is zero.
 * \return        #MBEDTLS_ERR_MPI_NEGATIVE_VALUE if \p N modulus is negative.
 */
int mbedtls_mpi_core_get_mont_r2_unsafe(mbedtls_mpi *X,
                                        const mbedtls_mpi *N, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Fill an integer with a number of random bytes.
 *
 * \param X        The destination MPI.
 * \param X_limbs  The number of limbs of \p X.
 * \param bytes    The number of random bytes to generate.
 * \param f_rng    The RNG function to use. This must not be \c NULL.
 * \param p_rng    The RNG parameter to be passed to \p f_rng. This may be
 *                 \c NULL if \p f_rng doesn't need a context argument.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if \p X does not have
 *                 enough room for \p bytes bytes.
 * \return         A negative error code on RNG failure.
 *
 * \note           The bytes obtained from the RNG are interpreted
 *                 as a big-endian representation of an MPI; this can
 *                 be relevant in applications like deterministic ECDSA.
 */
int mbedtls_mpi_core_fill_random(mbedtls_mpi_uint *X, size_t X_limbs,
                                 size_t bytes,
                                 int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                 void *p_rng, MinTlsDiagnostics* diagnostics);

/** Generate a random number uniformly in a range.
 *
 * This function generates a random number between \p min inclusive and
 * \p N exclusive.
 *
 * The procedure complies with RFC 6979 §3.3 (deterministic ECDSA)
 * when the RNG is a suitably parametrized instance of HMAC_DRBG
 * and \p min is \c 1.
 *
 * \note           There are `N - min` possible outputs. The lower bound
 *                 \p min can be reached, but the upper bound \p N cannot.
 *
 * \param X        The destination MPI, with \p limbs limbs.
 *                 It must not be aliased with \p N or otherwise overlap it.
 * \param min      The minimum value to return.
 * \param N        The upper bound of the range, exclusive, with \p limbs limbs.
 *                 In other words, this is one plus the maximum value to return.
 *                 \p N must be strictly larger than \p min.
 * \param limbs    The number of limbs of \p N and \p X.
 *                 This must not be 0.
 * \param f_rng    The RNG function to use. This must not be \c NULL.
 * \param p_rng    The RNG parameter to be passed to \p f_rng.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if the implementation was
 *                 unable to find a suitable value within a limited number
 *                 of attempts. This has a negligible probability if \p N
 *                 is significantly larger than \p min, which is the case
 *                 for all usual cryptographic applications.
 */
int mbedtls_mpi_core_random(mbedtls_mpi_uint *X,
                            mbedtls_mpi_uint min,
                            const mbedtls_mpi_uint *N,
                            size_t limbs,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Returns the number of limbs of working memory required for
 *                 a call to `mbedtls_mpi_core_exp_mod()`.
 *
 * \note           This will always be at least
 *                 `mbedtls_mpi_core_montmul_working_limbs(AN_limbs)`,
 *                 i.e. sufficient for a call to `mbedtls_mpi_core_montmul()`.
 *
 * \param AN_limbs The number of limbs in the input `A` and the modulus `N`
 *                 (they must be the same size) that will be given to
 *                 `mbedtls_mpi_core_exp_mod()`.
 * \param E_limbs  The number of limbs in the exponent `E` that will be given
 *                 to `mbedtls_mpi_core_exp_mod()`.
 *
 * \return         The number of limbs of working memory required by
 *                 `mbedtls_mpi_core_exp_mod()`.
 */
size_t mbedtls_mpi_core_exp_mod_working_limbs(size_t AN_limbs, size_t E_limbs);

/**
 * \brief            Perform a modular exponentiation with public or secret exponent:
 *                   X = A^E mod N, where \p A is already in Montgomery form.
 *
 * \warning          This function is not constant time with respect to \p E (the exponent).
 *
 * \p X may be aliased to \p A, but not to \p RR or \p E, even if \p E_limbs ==
 * \p AN_limbs.
 *
 * \param[out] X     The destination MPI, as a little endian array of length
 *                   \p AN_limbs.
 * \param[in] A      The base MPI, as a little endian array of length \p AN_limbs.
 *                   Must be in Montgomery form.
 * \param[in] N      The modulus, as a little endian array of length \p AN_limbs.
 * \param AN_limbs   The number of limbs in \p X, \p A, \p N, \p RR.
 * \param[in] E      The exponent, as a little endian array of length \p E_limbs.
 * \param E_limbs    The number of limbs in \p E.
 * \param[in] RR     The precomputed residue of 2^{2*biL} modulo N, as a little
 *                   endian array of length \p AN_limbs.
 * \param[in,out] T  Temporary storage of at least the number of limbs returned
 *                   by `mbedtls_mpi_core_exp_mod_working_limbs()`.
 *                   Its initial content is unused and its final content is
 *                   indeterminate.
 *                   It must not alias or otherwise overlap any of the other
 *                   parameters.
 *                   It is up to the caller to zeroize \p T when it is no
 *                   longer needed, and before freeing it if it was dynamically
 *                   allocated.
 */
void mbedtls_mpi_core_exp_mod_unsafe(mbedtls_mpi_uint *X,
                                     const mbedtls_mpi_uint *A,
                                     const mbedtls_mpi_uint *N, size_t AN_limbs,
                                     const mbedtls_mpi_uint *E, size_t E_limbs,
                                     const mbedtls_mpi_uint *RR,
                                     mbedtls_mpi_uint *T);

/**
 * \brief            Perform a modular exponentiation with secret exponent:
 *                   X = A^E mod N, where \p A is already in Montgomery form.
 *
 * \p X may be aliased to \p A, but not to \p RR or \p E, even if \p E_limbs ==
 * \p AN_limbs.
 *
 * \param[out] X     The destination MPI, as a little endian array of length
 *                   \p AN_limbs.
 * \param[in] A      The base MPI, as a little endian array of length \p AN_limbs.
 *                   Must be in Montgomery form.
 * \param[in] N      The modulus, as a little endian array of length \p AN_limbs.
 * \param AN_limbs   The number of limbs in \p X, \p A, \p N, \p RR.
 * \param[in] E      The exponent, as a little endian array of length \p E_limbs.
 * \param E_limbs    The number of limbs in \p E.
 * \param[in] RR     The precomputed residue of 2^{2*biL} modulo N, as a little
 *                   endian array of length \p AN_limbs.
 * \param[in,out] T  Temporary storage of at least the number of limbs returned
 *                   by `mbedtls_mpi_core_exp_mod_working_limbs()`.
 *                   Its initial content is unused and its final content is
 *                   indeterminate.
 *                   It must not alias or otherwise overlap any of the other
 *                   parameters.
 *                   It is up to the caller to zeroize \p T when it is no
 *                   longer needed, and before freeing it if it was dynamically
 *                   allocated.
 */
void mbedtls_mpi_core_exp_mod(mbedtls_mpi_uint *X,
                              const mbedtls_mpi_uint *A,
                              const mbedtls_mpi_uint *N, size_t AN_limbs,
                              const mbedtls_mpi_uint *E, size_t E_limbs,
                              const mbedtls_mpi_uint *RR,
                              mbedtls_mpi_uint *T);

/**
 * \brief Subtract unsigned integer from known-size large unsigned integers.
 *        Return the borrow.
 *
 * \param[out] X    The result of the subtraction.
 * \param[in] A     The left operand.
 * \param b         The unsigned scalar to subtract.
 * \param limbs     Number of limbs of \p X and \p A.
 *
 * \return          1 if `A < b`.
 *                  0 if `A >= b`.
 */
mbedtls_mpi_uint mbedtls_mpi_core_sub_int(mbedtls_mpi_uint *X,
                                          const mbedtls_mpi_uint *A,
                                          mbedtls_mpi_uint b,
                                          size_t limbs);

/**
 * \brief Determine if a given MPI has the value \c 0 in constant time with
 *        respect to the value (but not with respect to the number of limbs).
 *
 * \param[in] A   The MPI to test.
 * \param limbs   Number of limbs in \p A.
 *
 * \return        MBEDTLS_CT_FALSE if `A == 0`
 *                MBEDTLS_CT_TRUE  if `A != 0`.
 */
mbedtls_ct_condition_t mbedtls_mpi_core_check_zero_ct(const mbedtls_mpi_uint *A,
                                                      size_t limbs);

/**
 * \brief          Returns the number of limbs of working memory required for
 *                 a call to `mbedtls_mpi_core_montmul()`.
 *
 * \param AN_limbs The number of limbs in the input `A` and the modulus `N`
 *                 (they must be the same size) that will be given to
 *                 `mbedtls_mpi_core_montmul()` or one of the other functions
 *                 that specifies this as the amount of working memory needed.
 *
 * \return         The number of limbs of working memory required by
 *                 `mbedtls_mpi_core_montmul()` (or other similar function).
 */
static inline size_t mbedtls_mpi_core_montmul_working_limbs(size_t AN_limbs)
{
    return 2 * AN_limbs + 1;
}

/** Convert an MPI into Montgomery form.
 *
 * \p X may be aliased to \p A, but may not otherwise overlap it.
 *
 * \p X may not alias \p N (it is in canonical form, so must be strictly less
 * than \p N). Nor may it alias or overlap \p rr (this is unlikely to be
 * required in practice.)
 *
 * This function is a thin wrapper around `mbedtls_mpi_core_montmul()` that is
 * an alternative to calling `mbedtls_mpi_mod_raw_to_mont_rep()` when we
 * don't want to allocate memory.
 *
 * \param[out]    X         The result of the conversion.
 *                          Must have the same number of limbs as \p A.
 * \param[in]     A         The MPI to convert into Montgomery form.
 *                          Must have the same number of limbs as the modulus.
 * \param[in]     N         The address of the modulus, which gives the size of
 *                          the base `R` = 2^(biL*N->limbs).
 * \param[in]     AN_limbs  The number of limbs in \p X, \p A, \p N and \p rr.
 * \param         mm        The Montgomery constant for \p N: -N^-1 mod 2^biL.
 *                          This can be determined by calling
 *                          `mbedtls_mpi_core_montmul_init()`.
 * \param[in]     rr        The residue for `2^{2*n*biL} mod N`.
 * \param[in,out] T         Temporary storage of size at least
 *                          `mbedtls_mpi_core_montmul_working_limbs(AN_limbs)`
 *                          limbs.
 *                          Its initial content is unused and
 *                          its final content is indeterminate.
 *                          It must not alias or otherwise overlap any of the
 *                          other parameters.
 */
void mbedtls_mpi_core_to_mont_rep(mbedtls_mpi_uint *X,
                                  const mbedtls_mpi_uint *A,
                                  const mbedtls_mpi_uint *N,
                                  size_t AN_limbs,
                                  mbedtls_mpi_uint mm,
                                  const mbedtls_mpi_uint *rr,
                                  mbedtls_mpi_uint *T);

/** Convert an MPI from Montgomery form.
 *
 * \p X may be aliased to \p A, but may not otherwise overlap it.
 *
 * \p X may not alias \p N (it is in canonical form, so must be strictly less
 * than \p N).
 *
 * This function is a thin wrapper around `mbedtls_mpi_core_montmul()` that is
 * an alternative to calling `mbedtls_mpi_mod_raw_from_mont_rep()` when we
 * don't want to allocate memory.
 *
 * \param[out]    X         The result of the conversion.
 *                          Must have the same number of limbs as \p A.
 * \param[in]     A         The MPI to convert from Montgomery form.
 *                          Must have the same number of limbs as the modulus.
 * \param[in]     N         The address of the modulus, which gives the size of
 *                          the base `R` = 2^(biL*N->limbs).
 * \param[in]     AN_limbs  The number of limbs in \p X, \p A and \p N.
 * \param         mm        The Montgomery constant for \p N: -N^-1 mod 2^biL.
 *                          This can be determined by calling
 *                          `mbedtls_mpi_core_montmul_init()`.
 * \param[in,out] T         Temporary storage of size at least
 *                          `mbedtls_mpi_core_montmul_working_limbs(AN_limbs)`
 *                          limbs.
 *                          Its initial content is unused and
 *                          its final content is indeterminate.
 *                          It must not alias or otherwise overlap any of the
 *                          other parameters.
 */
void mbedtls_mpi_core_from_mont_rep(mbedtls_mpi_uint *X,
                                    const mbedtls_mpi_uint *A,
                                    const mbedtls_mpi_uint *N,
                                    size_t AN_limbs,
                                    mbedtls_mpi_uint mm,
                                    mbedtls_mpi_uint *T);

/** Compute GCD(A, N) and optionally the inverse of A mod N if it exists.
 *
 * Requires N to be odd, 0 <= A <= N and A_limbs <= N_limbs.
 * When I != NULL, N (the modulus) must be greater than 1.
 *
 * A and N may not alias each other.
 * When I == NULL (computing only the GCD), G may alias A or N.
 * When I != NULL (computing the modular inverse), G or I may alias A
 * but none of them may alias N (the modulus).
 *
 * If any of the above preconditions is not met, output values are unspecified.
 *
 * \param[out]    G       The GCD of \p A and \p N.
 *                        Must have the same number of limbs as \p N.
 * \param[out]    I       The inverse of \p A modulo \p N if it exists (that is,
 *                        if \p G above is 1 on exit); indeterminate otherwise.
 *                        This must either be NULL (to only compute the GCD),
 *                        or have the same number of limbs as \p N.
 * \param[in]     A       The 1st operand of GCD and number to invert.
 *                        This value must be less than or equal to \p N.
 * \param         A_limbs The number of limbs of \p A.
 *                        Must be less than or equal to \p N_limbs.
 * \param[in]     N       The 2nd operand of GCD and modulus for inversion.
 *                        This value must be odd.
 *                        If I != NULL this value must be greater than 1.
 * \param         N_limbs The number of limbs of \p N.
 * \param[in,out] T       Temporary storage of size at least 5 * N_limbs limbs,
 *                        or 4 * N_limbs if \p I is NULL (GCD only).
 *                        Its initial content is unused and
 *                        its final content is indeterminate.
 *                        It must not alias or otherwise overlap any of the
 *                        other parameters.
 */
void mbedtls_mpi_core_gcd_modinv_odd(mbedtls_mpi_uint *G,
                                     mbedtls_mpi_uint *I,
                                     const mbedtls_mpi_uint *A,
                                     size_t A_limbs,
                                     const mbedtls_mpi_uint *N,
                                     size_t N_limbs,
                                     mbedtls_mpi_uint *T);

#endif /* MBEDTLS_BIGNUM_CORE_H */

/* ===== Retained bignum_core_invasive.h ===== */

/**
 * \file bignum_core_invasive.h
 *
 * \brief Function declarations for invasive functions of bignum core.
 */
/**
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/bignum_core_invasive.h.
 */

#ifndef MBEDTLS_BIGNUM_CORE_INVASIVE_H
#define MBEDTLS_BIGNUM_CORE_INVASIVE_H


#endif /* MBEDTLS_BIGNUM_CORE_INVASIVE_H */

/* ===== Retained bignum_internal.h ===== */

/**
 * \file bignum_internal.h
 *
 * \brief Internal-only bignum public-key cryptosystem API.
 *
 * This file declares bignum-related functions that are to be used
 * only from within the Mbed TLS library itself.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/bignum_internal.h.
 */
#ifndef MBEDTLS_BIGNUM_INTERNAL_H
#define MBEDTLS_BIGNUM_INTERNAL_H

/**
 * \brief          Perform a modular exponentiation: X = A^E mod N
 *
 * \warning        This function is not constant time with respect to \p E (the exponent).
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 *                 This must not alias E or N.
 * \param A        The base of the exponentiation.
 *                 This must point to an initialized MPI.
 * \param E        The exponent MPI. This must point to an initialized MPI.
 * \param N        The base for the modular reduction. This must point to an
 *                 initialized MPI.
 * \param prec_RR  A helper MPI depending solely on \p N which can be used to
 *                 speed-up multiple modular exponentiations for the same value
 *                 of \p N. This may be \c NULL. If it is not \c NULL, it must
 *                 point to an initialized MPI. If it hasn't been used after
 *                 the call to mbedtls_mpi_init(), this function will compute
 *                 the helper value and store it in \p prec_RR for reuse on
 *                 subsequent calls to this function. Otherwise, the function
 *                 will assume that \p prec_RR holds the helper value set by a
 *                 previous call to mbedtls_mpi_exp_mod(), and reuse it.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if \c N is negative or
 *                 even, or if \c E is negative.
 * \return         Another negative error code on different kinds of failures.
 *
 */
int mbedtls_mpi_exp_mod_unsafe(mbedtls_mpi *X, const mbedtls_mpi *A,
                               const mbedtls_mpi *E, const mbedtls_mpi *N,
                               mbedtls_mpi *prec_RR, MinTlsDiagnostics* diagnostics);

/**
 * \brief          A wrapper around a constant time function to compute
 *                 GCD(A, N) and/or A^-1 mod N if it exists.
 *
 * \warning        Requires N to be odd, and 0 <= A <= N. Additionally, if
 *                 I != NULL, requires N > 1.
 *                 The wrapper part of this function is not constant time.
 *
 * \note           A and N must not alias each other.
 *                 When I == NULL (computing only the GCD), G can alias A or N.
 *                 When I != NULL (computing the modular inverse), G or I can
 *                 alias A, but neither of them can alias N (the modulus).
 *
 * \param[out] G   The GCD of \p A and \p N.
 *                 This may be NULL, to only compute I.
 * \param[out] I   The inverse of \p A modulo \p N if it exists (that is,
 *                 if \p G above is 1 on exit), in the range [1, \p N);
 *                 indeterminate otherwise.
 *                 This may be NULL, to only compute G.
 * \param[in] A    The 1st operand of GCD and number to invert.
 *                 This value must be less than or equal to \p N.
 * \param[in] N    The 2nd operand of GCD and modulus for inversion.
 *                 Must be odd or the results are indeterminate.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if preconditions were not
 *                 met.
 */
int mbedtls_mpi_gcd_modinv_odd(mbedtls_mpi *G,
                               mbedtls_mpi *I,
                               const mbedtls_mpi *A,
                               const mbedtls_mpi *N, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Modular inverse: X = A^-1 mod N with N odd
 *
 * \param[out] X   The inverse of \p A modulo \p N in the range [1, \p N)
 *                 on success; indeterminate otherwise.
 * \param[in] A    The number to invert.
 * \param[in] N    The modulus. Must be odd and greater than 1.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if preconditions were not
 *                 met.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if A is not invertible mod N.
 */
int mbedtls_mpi_inv_mod_odd(mbedtls_mpi *X,
                            const mbedtls_mpi *A,
                            const mbedtls_mpi *N, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Modular inverse: X = A^-1 mod N with N even,
 *                 A odd and 1 < A < N.
 *
 * \param[out] X   The inverse of \p A modulo \p N in the range [1, \p N)
 *                 on success; indeterminate otherwise.
 * \param[in] A    The number to invert. Must be odd, greated than 1
 *                 and less than \p N.
 * \param[in] N    The modulus. Must be even and greater than 1.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if preconditions were not
 *                 met.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if A is not invertible mod N.
 */
int mbedtls_mpi_inv_mod_even_in_range(mbedtls_mpi *X,
                                      mbedtls_mpi const *A,
                                      mbedtls_mpi const *N, MinTlsDiagnostics* diagnostics);

#endif /* bignum_internal.h */

/* ===== Retained bn_mul.h ===== */

/**
 * \file bn_mul.h
 *
 * \brief Multi-precision integer library
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/bn_mul.h.
 */
/*
 *      Multiply source vector [s] with b, add result
 *       to destination vector [d] and set carry c.
 *
 *      Currently supports:
 *
 *         . IA-32 (386+)         . AMD64 / EM64T
 *         . IA-32 (SSE2)         . Motorola 68000
 *         . PowerPC, 32-bit      . MicroBlaze
 *         . PowerPC, 64-bit      . TriCore
 *         . SPARC v8             . ARM v3+
 *         . Alpha                . MIPS32
 *         . C, longlong          . C, generic
 */
#ifndef MBEDTLS_BN_MUL_H
#define MBEDTLS_BN_MUL_H



/*
 * Conversion macros for embedded constants:
 * build lists of mbedtls_mpi_uint's from lists of unsigned char's grouped by 8, 4 or 2
 */
#if defined(MBEDTLS_HAVE_INT32)

#define MBEDTLS_BYTES_TO_T_UINT_4(a, b, c, d)               \
    ((mbedtls_mpi_uint) (a) <<  0) |                        \
    ((mbedtls_mpi_uint) (b) <<  8) |                        \
    ((mbedtls_mpi_uint) (c) << 16) |                        \
    ((mbedtls_mpi_uint) (d) << 24)

#define MBEDTLS_BYTES_TO_T_UINT_2(a, b)                   \
    MBEDTLS_BYTES_TO_T_UINT_4(a, b, 0, 0)

#define MBEDTLS_BYTES_TO_T_UINT_8(a, b, c, d, e, f, g, h) \
    MBEDTLS_BYTES_TO_T_UINT_4(a, b, c, d),                \
    MBEDTLS_BYTES_TO_T_UINT_4(e, f, g, h)

#else

#define MBEDTLS_BYTES_TO_T_UINT_8(a, b, c, d, e, f, g, h)   \
    ((mbedtls_mpi_uint) (a) <<  0) |                        \
    ((mbedtls_mpi_uint) (b) <<  8) |                        \
    ((mbedtls_mpi_uint) (c) << 16) |                        \
    ((mbedtls_mpi_uint) (d) << 24) |                        \
    ((mbedtls_mpi_uint) (e) << 32) |                        \
    ((mbedtls_mpi_uint) (f) << 40) |                        \
    ((mbedtls_mpi_uint) (g) << 48) |                        \
    ((mbedtls_mpi_uint) (h) << 56)

#define MBEDTLS_BYTES_TO_T_UINT_4(a, b, c, d)             \
    MBEDTLS_BYTES_TO_T_UINT_8(a, b, c, d, 0, 0, 0, 0)

#define MBEDTLS_BYTES_TO_T_UINT_2(a, b)                   \
    MBEDTLS_BYTES_TO_T_UINT_8(a, b, 0, 0, 0, 0, 0, 0)

#endif /* bits in mbedtls_mpi_uint */

/* *INDENT-OFF* */

/* armcc5 --gnu defines __GNUC__ but doesn't support GNU's extended asm */
#if defined(__GNUC__) && \
    ( !defined(__ARMCC_VERSION) || __ARMCC_VERSION >= 6000000 )

/*
 * GCC < 5.0 treated the x86 ebx (which is used for the GOT) as a
 * fixed reserved register when building as PIC, leading to errors
 * like: bn_mul.h:46:13: error: PIC register clobbered by 'ebx' in 'asm'
 *
 * This is fixed by an improved register allocator in GCC 5+. From the
 * release notes:
 * Register allocation improvements: Reuse of the PIC hard register,
 * instead of using a fixed register, was implemented on x86/x86-64
 * targets. This improves generated PIC code performance as more hard
 * registers can be used.
 */
#if defined(__GNUC__) && __GNUC__ < 5 && defined(__PIC__)
#define MULADDC_CANNOT_USE_EBX
#endif

/*
 * Disable use of the i386 assembly code below if option -O0, to disable all
 * compiler optimisations, is passed, detected with __OPTIMIZE__
 * This is done as the number of registers used in the assembly code doesn't
 * work with the -O0 option.
 */
#if defined(__i386__) && defined(__OPTIMIZE__) && !defined(MULADDC_CANNOT_USE_EBX)

#define MULADDC_X1_INIT                     \
    { mbedtls_mpi_uint t;                   \
    asm(                                    \
        "movl   %%ebx, %0           \n\t"   \
        "movl   %5, %%esi           \n\t"   \
        "movl   %6, %%edi           \n\t"   \
        "movl   %7, %%ecx           \n\t"   \
        "movl   %8, %%ebx           \n\t"

#define MULADDC_X1_CORE                     \
        "lodsl                      \n\t"   \
        "mull   %%ebx               \n\t"   \
        "addl   %%ecx,   %%eax      \n\t"   \
        "adcl   $0,      %%edx      \n\t"   \
        "addl   (%%edi), %%eax      \n\t"   \
        "adcl   $0,      %%edx      \n\t"   \
        "movl   %%edx,   %%ecx      \n\t"   \
        "stosl                      \n\t"

#define MULADDC_X1_STOP                                 \
        "movl   %4, %%ebx       \n\t"                   \
        "movl   %%ecx, %1       \n\t"                   \
        "movl   %%edi, %2       \n\t"                   \
        "movl   %%esi, %3       \n\t"                   \
        : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ebx", "ecx", "edx", "esi", "edi"      \
    ); }

#if defined(MBEDTLS_HAVE_SSE2)

#define MULADDC_X8_INIT MULADDC_X1_INIT

#define MULADDC_X8_CORE                         \
        "movd     %%ecx,     %%mm1      \n\t"   \
        "movd     %%ebx,     %%mm0      \n\t"   \
        "movd     (%%edi),   %%mm3      \n\t"   \
        "paddq    %%mm3,     %%mm1      \n\t"   \
        "movd     (%%esi),   %%mm2      \n\t"   \
        "pmuludq  %%mm0,     %%mm2      \n\t"   \
        "movd     4(%%esi),  %%mm4      \n\t"   \
        "pmuludq  %%mm0,     %%mm4      \n\t"   \
        "movd     8(%%esi),  %%mm6      \n\t"   \
        "pmuludq  %%mm0,     %%mm6      \n\t"   \
        "movd     12(%%esi), %%mm7      \n\t"   \
        "pmuludq  %%mm0,     %%mm7      \n\t"   \
        "paddq    %%mm2,     %%mm1      \n\t"   \
        "movd     4(%%edi),  %%mm3      \n\t"   \
        "paddq    %%mm4,     %%mm3      \n\t"   \
        "movd     8(%%edi),  %%mm5      \n\t"   \
        "paddq    %%mm6,     %%mm5      \n\t"   \
        "movd     12(%%edi), %%mm4      \n\t"   \
        "paddq    %%mm4,     %%mm7      \n\t"   \
        "movd     %%mm1,     (%%edi)    \n\t"   \
        "movd     16(%%esi), %%mm2      \n\t"   \
        "pmuludq  %%mm0,     %%mm2      \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "movd     20(%%esi), %%mm4      \n\t"   \
        "pmuludq  %%mm0,     %%mm4      \n\t"   \
        "paddq    %%mm3,     %%mm1      \n\t"   \
        "movd     24(%%esi), %%mm6      \n\t"   \
        "pmuludq  %%mm0,     %%mm6      \n\t"   \
        "movd     %%mm1,     4(%%edi)   \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "movd     28(%%esi), %%mm3      \n\t"   \
        "pmuludq  %%mm0,     %%mm3      \n\t"   \
        "paddq    %%mm5,     %%mm1      \n\t"   \
        "movd     16(%%edi), %%mm5      \n\t"   \
        "paddq    %%mm5,     %%mm2      \n\t"   \
        "movd     %%mm1,     8(%%edi)   \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "paddq    %%mm7,     %%mm1      \n\t"   \
        "movd     20(%%edi), %%mm5      \n\t"   \
        "paddq    %%mm5,     %%mm4      \n\t"   \
        "movd     %%mm1,     12(%%edi)  \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "paddq    %%mm2,     %%mm1      \n\t"   \
        "movd     24(%%edi), %%mm5      \n\t"   \
        "paddq    %%mm5,     %%mm6      \n\t"   \
        "movd     %%mm1,     16(%%edi)  \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "paddq    %%mm4,     %%mm1      \n\t"   \
        "movd     28(%%edi), %%mm5      \n\t"   \
        "paddq    %%mm5,     %%mm3      \n\t"   \
        "movd     %%mm1,     20(%%edi)  \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "paddq    %%mm6,     %%mm1      \n\t"   \
        "movd     %%mm1,     24(%%edi)  \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "paddq    %%mm3,     %%mm1      \n\t"   \
        "movd     %%mm1,     28(%%edi)  \n\t"   \
        "addl     $32,       %%edi      \n\t"   \
        "addl     $32,       %%esi      \n\t"   \
        "psrlq    $32,       %%mm1      \n\t"   \
        "movd     %%mm1,     %%ecx      \n\t"

#define MULADDC_X8_STOP                 \
        "emms                   \n\t"   \
        "movl   %4, %%ebx       \n\t"   \
        "movl   %%ecx, %1       \n\t"   \
        "movl   %%edi, %2       \n\t"   \
        "movl   %%esi, %3       \n\t"   \
        : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ebx", "ecx", "edx", "esi", "edi"      \
    ); }                                                \

#endif /* SSE2 */

#endif /* i386 */

#if defined(__amd64__) || defined (__x86_64__)

#define MULADDC_X1_INIT                        \
    asm(                                    \
        "xorq   %%r8, %%r8\n"

#define MULADDC_X1_CORE                        \
        "movq   (%%rsi), %%rax\n"           \
        "mulq   %%rbx\n"                    \
        "addq   $8, %%rsi\n"                \
        "addq   %%rcx, %%rax\n"             \
        "movq   %%r8, %%rcx\n"              \
        "adcq   $0, %%rdx\n"                \
        "nop    \n"                         \
        "addq   %%rax, (%%rdi)\n"           \
        "adcq   %%rdx, %%rcx\n"             \
        "addq   $8, %%rdi\n"

#define MULADDC_X1_STOP                                              \
        : "+c" (c), "+D" (d), "+S" (s), "+m" (*(uint64_t (*)[16]) d) \
        : "b" (b), "m" (*(const uint64_t (*)[16]) s)                 \
        : "rax", "rdx", "r8"                                         \
    );

#endif /* AMD64 */

// The following assembly code assumes that a pointer will fit in a 64-bit register
// (including ILP32 __aarch64__ ABIs such as on watchOS, hence the 2^32 - 1)
#if defined(__aarch64__) && (UINTPTR_MAX == 0xfffffffful || UINTPTR_MAX == 0xfffffffffffffffful)

/*
 * There are some issues around different compilers requiring different constraint
 * syntax for updating pointers from assembly code (see notes for
 * MBEDTLS_ASM_AARCH64_PTR_CONSTRAINT in common.h), especially on aarch64_32 (aka ILP32).
 *
 * For this reason we cast the pointers to/from uintptr_t here.
 */
#define MULADDC_X1_INIT             \
    do { uintptr_t muladdc_d = (uintptr_t) d, muladdc_s = (uintptr_t) s; asm(

#define MULADDC_X1_CORE             \
        "ldr x4, [%x2], #8  \n\t"   \
        "ldr x5, [%x1]      \n\t"   \
        "mul x6, x4, %4     \n\t"   \
        "umulh x7, x4, %4   \n\t"   \
        "adds x5, x5, x6    \n\t"   \
        "adc x7, x7, xzr    \n\t"   \
        "adds x5, x5, %0    \n\t"   \
        "adc %0, x7, xzr    \n\t"   \
        "str x5, [%x1], #8  \n\t"

#define MULADDC_X1_STOP                                                 \
         : "+r" (c),                                                    \
           "+r" (muladdc_d),                                            \
           "+r" (muladdc_s),                                            \
           "+m" (*(uint64_t (*)[16]) d)                                 \
         : "r" (b), "m" (*(const uint64_t (*)[16]) s)                   \
         : "x4", "x5", "x6", "x7", "cc"                                 \
    ); d = (mbedtls_mpi_uint *)muladdc_d; s = (mbedtls_mpi_uint *)muladdc_s; } while (0);

#endif /* Aarch64 */

#if defined(__mc68020__) || defined(__mcpu32__)

#define MULADDC_X1_INIT                 \
    asm(                                \
        "movl   %3, %%a2        \n\t"   \
        "movl   %4, %%a3        \n\t"   \
        "movl   %5, %%d3        \n\t"   \
        "movl   %6, %%d2        \n\t"   \
        "moveq  #0, %%d0        \n\t"

#define MULADDC_X1_CORE                 \
        "movel  %%a2@+, %%d1    \n\t"   \
        "mulul  %%d2, %%d4:%%d1 \n\t"   \
        "addl   %%d3, %%d1      \n\t"   \
        "addxl  %%d0, %%d4      \n\t"   \
        "moveq  #0,   %%d3      \n\t"   \
        "addl   %%d1, %%a3@+    \n\t"   \
        "addxl  %%d4, %%d3      \n\t"

#define MULADDC_X1_STOP                 \
        "movl   %%d3, %0        \n\t"   \
        "movl   %%a3, %1        \n\t"   \
        "movl   %%a2, %2        \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "d0", "d1", "d2", "d3", "d4", "a2", "a3"  \
    );

#define MULADDC_X8_INIT MULADDC_X1_INIT

#define MULADDC_X8_CORE                     \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d4:%%d1  \n\t"   \
        "addxl  %%d3,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d4       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d3:%%d1  \n\t"   \
        "addxl  %%d4,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d3       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d4:%%d1  \n\t"   \
        "addxl  %%d3,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d4       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d3:%%d1  \n\t"   \
        "addxl  %%d4,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d3       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d4:%%d1  \n\t"   \
        "addxl  %%d3,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d4       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d3:%%d1  \n\t"   \
        "addxl  %%d4,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d3       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d4:%%d1  \n\t"   \
        "addxl  %%d3,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d4       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "movel  %%a2@+,  %%d1       \n\t"   \
        "mulul  %%d2,    %%d3:%%d1  \n\t"   \
        "addxl  %%d4,    %%d1       \n\t"   \
        "addxl  %%d0,    %%d3       \n\t"   \
        "addl   %%d1,    %%a3@+     \n\t"   \
        "addxl  %%d0,    %%d3       \n\t"

#define MULADDC_X8_STOP MULADDC_X1_STOP

#endif /* MC68000 */

#if defined(__powerpc64__) || defined(__ppc64__)

#if defined(__MACH__) && defined(__APPLE__)

#define MULADDC_X1_INIT                     \
    asm(                                    \
        "ld     r3, %3              \n\t"   \
        "ld     r4, %4              \n\t"   \
        "ld     r5, %5              \n\t"   \
        "ld     r6, %6              \n\t"   \
        "addi   r3, r3, -8          \n\t"   \
        "addi   r4, r4, -8          \n\t"   \
        "addic  r5, r5,  0          \n\t"

#define MULADDC_X1_CORE                     \
        "ldu    r7, 8(r3)           \n\t"   \
        "mulld  r8, r7, r6          \n\t"   \
        "mulhdu r9, r7, r6          \n\t"   \
        "adde   r8, r8, r5          \n\t"   \
        "ld     r7, 8(r4)           \n\t"   \
        "addze  r5, r9              \n\t"   \
        "addc   r8, r8, r7          \n\t"   \
        "stdu   r8, 8(r4)           \n\t"

#define MULADDC_X1_STOP                     \
        "addze  r5, r5              \n\t"   \
        "addi   r4, r4, 8           \n\t"   \
        "addi   r3, r3, 8           \n\t"   \
        "std    r5, %0              \n\t"   \
        "std    r4, %1              \n\t"   \
        "std    r3, %2              \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "r3", "r4", "r5", "r6", "r7", "r8", "r9"  \
    );

#else

#define MULADDC_X1_INIT                     \
    asm(                                    \
        "ld     %%r3, %3            \n\t"   \
        "ld     %%r4, %4            \n\t"   \
        "ld     %%r5, %5            \n\t"   \
        "ld     %%r6, %6            \n\t"   \
        "addi   %%r3, %%r3, -8      \n\t"   \
        "addi   %%r4, %%r4, -8      \n\t"   \
        "addic  %%r5, %%r5,  0      \n\t"

#define MULADDC_X1_CORE                     \
        "ldu    %%r7, 8(%%r3)       \n\t"   \
        "mulld  %%r8, %%r7, %%r6    \n\t"   \
        "mulhdu %%r9, %%r7, %%r6    \n\t"   \
        "adde   %%r8, %%r8, %%r5    \n\t"   \
        "ld     %%r7, 8(%%r4)       \n\t"   \
        "addze  %%r5, %%r9          \n\t"   \
        "addc   %%r8, %%r8, %%r7    \n\t"   \
        "stdu   %%r8, 8(%%r4)       \n\t"

#define MULADDC_X1_STOP                     \
        "addze  %%r5, %%r5          \n\t"   \
        "addi   %%r4, %%r4, 8       \n\t"   \
        "addi   %%r3, %%r3, 8       \n\t"   \
        "std    %%r5, %0            \n\t"   \
        "std    %%r4, %1            \n\t"   \
        "std    %%r3, %2            \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "r3", "r4", "r5", "r6", "r7", "r8", "r9"  \
    );

#endif /* __MACH__ && __APPLE__ */

#elif defined(__powerpc__) || defined(__ppc__) /* end PPC64/begin PPC32  */

#if defined(__MACH__) && defined(__APPLE__)

#define MULADDC_X1_INIT                 \
    asm(                                \
        "lwz    r3, %3          \n\t"   \
        "lwz    r4, %4          \n\t"   \
        "lwz    r5, %5          \n\t"   \
        "lwz    r6, %6          \n\t"   \
        "addi   r3, r3, -4      \n\t"   \
        "addi   r4, r4, -4      \n\t"   \
        "addic  r5, r5,  0      \n\t"

#define MULADDC_X1_CORE                 \
        "lwzu   r7, 4(r3)       \n\t"   \
        "mullw  r8, r7, r6      \n\t"   \
        "mulhwu r9, r7, r6      \n\t"   \
        "adde   r8, r8, r5      \n\t"   \
        "lwz    r7, 4(r4)       \n\t"   \
        "addze  r5, r9          \n\t"   \
        "addc   r8, r8, r7      \n\t"   \
        "stwu   r8, 4(r4)       \n\t"

#define MULADDC_X1_STOP                 \
        "addze  r5, r5          \n\t"   \
        "addi   r4, r4, 4       \n\t"   \
        "addi   r3, r3, 4       \n\t"   \
        "stw    r5, %0          \n\t"   \
        "stw    r4, %1          \n\t"   \
        "stw    r3, %2          \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "r3", "r4", "r5", "r6", "r7", "r8", "r9"  \
    );

#else

#define MULADDC_X1_INIT                     \
    asm(                                    \
        "lwz    %%r3, %3            \n\t"   \
        "lwz    %%r4, %4            \n\t"   \
        "lwz    %%r5, %5            \n\t"   \
        "lwz    %%r6, %6            \n\t"   \
        "addi   %%r3, %%r3, -4      \n\t"   \
        "addi   %%r4, %%r4, -4      \n\t"   \
        "addic  %%r5, %%r5,  0      \n\t"

#define MULADDC_X1_CORE                     \
        "lwzu   %%r7, 4(%%r3)       \n\t"   \
        "mullw  %%r8, %%r7, %%r6    \n\t"   \
        "mulhwu %%r9, %%r7, %%r6    \n\t"   \
        "adde   %%r8, %%r8, %%r5    \n\t"   \
        "lwz    %%r7, 4(%%r4)       \n\t"   \
        "addze  %%r5, %%r9          \n\t"   \
        "addc   %%r8, %%r8, %%r7    \n\t"   \
        "stwu   %%r8, 4(%%r4)       \n\t"

#define MULADDC_X1_STOP                     \
        "addze  %%r5, %%r5          \n\t"   \
        "addi   %%r4, %%r4, 4       \n\t"   \
        "addi   %%r3, %%r3, 4       \n\t"   \
        "stw    %%r5, %0            \n\t"   \
        "stw    %%r4, %1            \n\t"   \
        "stw    %%r3, %2            \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "r3", "r4", "r5", "r6", "r7", "r8", "r9"  \
    );

#endif /* __MACH__ && __APPLE__ */

#endif /* PPC32 */

/*
 * The Sparc(64) assembly is reported to be broken.
 * Disable it for now, until we're able to fix it.
 */
#if 0 && defined(__sparc__)
#if defined(__sparc64__)

#define MULADDC_X1_INIT                                 \
    asm(                                                \
                "ldx     %3, %%o0               \n\t"   \
                "ldx     %4, %%o1               \n\t"   \
                "ld      %5, %%o2               \n\t"   \
                "ld      %6, %%o3               \n\t"

#define MULADDC_X1_CORE                                 \
                "ld      [%%o0], %%o4           \n\t"   \
                "inc     4, %%o0                \n\t"   \
                "ld      [%%o1], %%o5           \n\t"   \
                "umul    %%o3, %%o4, %%o4       \n\t"   \
                "addcc   %%o4, %%o2, %%o4       \n\t"   \
                "rd      %%y, %%g1              \n\t"   \
                "addx    %%g1, 0, %%g1          \n\t"   \
                "addcc   %%o4, %%o5, %%o4       \n\t"   \
                "st      %%o4, [%%o1]           \n\t"   \
                "addx    %%g1, 0, %%o2          \n\t"   \
                "inc     4, %%o1                \n\t"

#define MULADDC_X1_STOP                                 \
                "st      %%o2, %0               \n\t"   \
                "stx     %%o1, %1               \n\t"   \
                "stx     %%o0, %2               \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)          \
        : "m" (s), "m" (d), "m" (c), "m" (b)    \
        : "g1", "o0", "o1", "o2", "o3", "o4",   \
          "o5"                                  \
        );

#else

#define MULADDC_X1_INIT                                 \
    asm(                                                \
                "ld      %3, %%o0               \n\t"   \
                "ld      %4, %%o1               \n\t"   \
                "ld      %5, %%o2               \n\t"   \
                "ld      %6, %%o3               \n\t"

#define MULADDC_X1_CORE                                 \
                "ld      [%%o0], %%o4           \n\t"   \
                "inc     4, %%o0                \n\t"   \
                "ld      [%%o1], %%o5           \n\t"   \
                "umul    %%o3, %%o4, %%o4       \n\t"   \
                "addcc   %%o4, %%o2, %%o4       \n\t"   \
                "rd      %%y, %%g1              \n\t"   \
                "addx    %%g1, 0, %%g1          \n\t"   \
                "addcc   %%o4, %%o5, %%o4       \n\t"   \
                "st      %%o4, [%%o1]           \n\t"   \
                "addx    %%g1, 0, %%o2          \n\t"   \
                "inc     4, %%o1                \n\t"

#define MULADDC_X1_STOP                                 \
                "st      %%o2, %0               \n\t"   \
                "st      %%o1, %1               \n\t"   \
                "st      %%o0, %2               \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)          \
        : "m" (s), "m" (d), "m" (c), "m" (b)    \
        : "g1", "o0", "o1", "o2", "o3", "o4",   \
          "o5"                                  \
        );

#endif /* __sparc64__ */
#endif /* __sparc__ */

#if defined(__microblaze__) || defined(microblaze)

#define MULADDC_X1_INIT                 \
    asm(                                \
        "lwi   r3,   %3         \n\t"   \
        "lwi   r4,   %4         \n\t"   \
        "lwi   r5,   %5         \n\t"   \
        "lwi   r6,   %6         \n\t"   \
        "andi  r7,   r6, 0xffff \n\t"   \
        "bsrli r6,   r6, 16     \n\t"

#if(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define MULADDC_LHUI                    \
        "lhui  r9,   r3,   0    \n\t"   \
        "addi  r3,   r3,   2    \n\t"   \
        "lhui  r8,   r3,   0    \n\t"
#else
#define MULADDC_LHUI                    \
        "lhui  r8,   r3,   0    \n\t"   \
        "addi  r3,   r3,   2    \n\t"   \
        "lhui  r9,   r3,   0    \n\t"
#endif

#define MULADDC_X1_CORE                    \
        MULADDC_LHUI                    \
        "addi  r3,   r3,   2    \n\t"   \
        "mul   r10,  r9,  r6    \n\t"   \
        "mul   r11,  r8,  r7    \n\t"   \
        "mul   r12,  r9,  r7    \n\t"   \
        "mul   r13,  r8,  r6    \n\t"   \
        "bsrli  r8, r10,  16    \n\t"   \
        "bsrli  r9, r11,  16    \n\t"   \
        "add   r13, r13,  r8    \n\t"   \
        "add   r13, r13,  r9    \n\t"   \
        "bslli r10, r10,  16    \n\t"   \
        "bslli r11, r11,  16    \n\t"   \
        "add   r12, r12, r10    \n\t"   \
        "addc  r13, r13,  r0    \n\t"   \
        "add   r12, r12, r11    \n\t"   \
        "addc  r13, r13,  r0    \n\t"   \
        "lwi   r10,  r4,   0    \n\t"   \
        "add   r12, r12, r10    \n\t"   \
        "addc  r13, r13,  r0    \n\t"   \
        "add   r12, r12,  r5    \n\t"   \
        "addc   r5, r13,  r0    \n\t"   \
        "swi   r12,  r4,   0    \n\t"   \
        "addi   r4,  r4,   4    \n\t"

#define MULADDC_X1_STOP                 \
        "swi   r5,   %0         \n\t"   \
        "swi   r4,   %1         \n\t"   \
        "swi   r3,   %2         \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "r3", "r4", "r5", "r6", "r7", "r8",       \
          "r9", "r10", "r11", "r12", "r13"          \
    );

#endif /* MicroBlaze */

#if defined(__tricore__)

#define MULADDC_X1_INIT                         \
    asm(                                        \
        "ld.a   %%a2, %3                \n\t"   \
        "ld.a   %%a3, %4                \n\t"   \
        "ld.w   %%d4, %5                \n\t"   \
        "ld.w   %%d1, %6                \n\t"   \
        "xor    %%d5, %%d5              \n\t"

#define MULADDC_X1_CORE                         \
        "ld.w   %%d0,   [%%a2+]         \n\t"   \
        "madd.u %%e2, %%e4, %%d0, %%d1  \n\t"   \
        "ld.w   %%d0,   [%%a3]          \n\t"   \
        "addx   %%d2,    %%d2,  %%d0    \n\t"   \
        "addc   %%d3,    %%d3,    0     \n\t"   \
        "mov    %%d4,    %%d3           \n\t"   \
        "st.w  [%%a3+],  %%d2           \n\t"

#define MULADDC_X1_STOP                         \
        "st.w   %0, %%d4                \n\t"   \
        "st.a   %1, %%a3                \n\t"   \
        "st.a   %2, %%a2                \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)          \
        : "m" (s), "m" (d), "m" (c), "m" (b)    \
        : "d0", "d1", "e2", "d4", "a2", "a3"    \
    );

#endif /* TriCore */

#if defined(__arm__)

#if defined(__thumb__) && !defined(__thumb2__)
#if defined(MBEDTLS_COMPILER_IS_GCC)
/*
 * Thumb 1 ISA. This code path has only been tested successfully on gcc;
 * it does not compile on clang or armclang.
 */

#if !defined(__OPTIMIZE__) && defined(__GNUC__)
/*
 * Note, gcc -O0 by default uses r7 for the frame pointer, so it complains about
 * our use of r7 below, unless -fomit-frame-pointer is passed.
 *
 * On the other hand, -fomit-frame-pointer is implied by any -Ox options with
 * x !=0, which we can detect using __OPTIMIZE__ (which is also defined by
 * clang and armcc5 under the same conditions).
 *
 * If gcc needs to use r7, we use r1 as a scratch register and have a few extra
 * instructions to preserve/restore it; otherwise, we can use r7 and avoid
 * the preserve/restore overhead.
 */
#define MULADDC_SCRATCH              "RS .req r1         \n\t"
#define MULADDC_PRESERVE_SCRATCH     "mov    r10, r1     \n\t"
#define MULADDC_RESTORE_SCRATCH      "mov    r1, r10     \n\t"
#define MULADDC_SCRATCH_CLOBBER      "r10"
#else
#define MULADDC_SCRATCH              "RS .req r7         \n\t"
#define MULADDC_PRESERVE_SCRATCH     ""
#define MULADDC_RESTORE_SCRATCH      ""
#define MULADDC_SCRATCH_CLOBBER      "r7"
#endif /* !defined(__OPTIMIZE__) && defined(__GNUC__) */

#define MULADDC_X1_INIT                                 \
    asm(                                                \
    MULADDC_SCRATCH                                     \
            "ldr    r0, %3                      \n\t"   \
            "ldr    r1, %4                      \n\t"   \
            "ldr    r2, %5                      \n\t"   \
            "ldr    r3, %6                      \n\t"   \
            "lsr    r4, r3, #16                 \n\t"   \
            "mov    r9, r4                      \n\t"   \
            "lsl    r4, r3, #16                 \n\t"   \
            "lsr    r4, r4, #16                 \n\t"   \
            "mov    r8, r4                      \n\t"   \

#define MULADDC_X1_CORE                                 \
            MULADDC_PRESERVE_SCRATCH                    \
            "ldmia  r0!, {r6}                   \n\t"   \
            "lsr    RS, r6, #16                 \n\t"   \
            "lsl    r6, r6, #16                 \n\t"   \
            "lsr    r6, r6, #16                 \n\t"   \
            "mov    r4, r8                      \n\t"   \
            "mul    r4, r6                      \n\t"   \
            "mov    r3, r9                      \n\t"   \
            "mul    r6, r3                      \n\t"   \
            "mov    r5, r9                      \n\t"   \
            "mul    r5, RS                      \n\t"   \
            "mov    r3, r8                      \n\t"   \
            "mul    RS, r3                      \n\t"   \
            "lsr    r3, r6, #16                 \n\t"   \
            "add    r5, r5, r3                  \n\t"   \
            "lsr    r3, RS, #16                 \n\t"   \
            "add    r5, r5, r3                  \n\t"   \
            "add    r4, r4, r2                  \n\t"   \
            "mov    r2, #0                      \n\t"   \
            "adc    r5, r2                      \n\t"   \
            "lsl    r3, r6, #16                 \n\t"   \
            "add    r4, r4, r3                  \n\t"   \
            "adc    r5, r2                      \n\t"   \
            "lsl    r3, RS, #16                 \n\t"   \
            "add    r4, r4, r3                  \n\t"   \
            "adc    r5, r2                      \n\t"   \
            MULADDC_RESTORE_SCRATCH                     \
            "ldr    r3, [r1]                    \n\t"   \
            "add    r4, r4, r3                  \n\t"   \
            "adc    r2, r5                      \n\t"   \
            "stmia  r1!, {r4}                   \n\t"

#define MULADDC_X1_STOP                                 \
            "str    r2, %0                      \n\t"   \
            "str    r1, %1                      \n\t"   \
            "str    r0, %2                      \n\t"   \
         : "=m" (c),  "=m" (d), "=m" (s)        \
         : "m" (s), "m" (d), "m" (c), "m" (b)   \
         : "r0", "r1", "r2", "r3", "r4", "r5",  \
           "r6", MULADDC_SCRATCH_CLOBBER, "r8", "r9", "cc" \
         );
#endif /* !defined(__ARMCC_VERSION) && !defined(__clang__) */

#elif (__ARM_ARCH >= 6) && \
    defined (__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
/* Armv6-M (or later) with DSP Instruction Set Extensions.
 * Requires support for either Thumb 2 or Arm ISA.
 */

#define MULADDC_X1_INIT                            \
    {                                              \
        mbedtls_mpi_uint tmp_a, tmp_b;             \
        asm volatile (

#define MULADDC_X1_CORE                                         \
           ".p2align  2                                 \n\t"   \
            "ldr      %[a], [%[in]], #4                 \n\t"   \
            "ldr      %[b], [%[acc]]                    \n\t"   \
            "umaal    %[b], %[carry], %[scalar], %[a]   \n\t"   \
            "str      %[b], [%[acc]], #4                \n\t"

#define MULADDC_X1_STOP                                      \
            : [a]      "=&r" (tmp_a),                        \
              [b]      "=&r" (tmp_b),                        \
              [in]     "+r"  (s),                            \
              [acc]    "+r"  (d),                            \
              [carry]  "+l"  (c)                             \
            : [scalar] "r"   (b)                             \
            : "memory"                                       \
        );                                                   \
    }

#define MULADDC_X2_INIT                              \
    {                                                \
        mbedtls_mpi_uint tmp_a0, tmp_b0;             \
        mbedtls_mpi_uint tmp_a1, tmp_b1;             \
        asm volatile (

            /* - Make sure loop is 4-byte aligned to avoid stalls
             *   upon repeated non-word aligned instructions in
             *   some microarchitectures.
             * - Don't use ldm with post-increment or back-to-back
             *   loads with post-increment and same address register
             *   to avoid stalls on some microarchitectures.
             * - Bunch loads and stores to reduce latency on some
             *   microarchitectures. E.g., on Cortex-M4, the first
             *   in a series of load/store operations has latency
             *   2 cycles, while subsequent loads/stores are single-cycle. */
#define MULADDC_X2_CORE                                           \
           ".p2align  2                                   \n\t"   \
            "ldr      %[a0], [%[in]],  #+8                \n\t"   \
            "ldr      %[b0], [%[acc]], #+8                \n\t"   \
            "ldr      %[a1], [%[in],  #-4]                \n\t"   \
            "ldr      %[b1], [%[acc], #-4]                \n\t"   \
            "umaal    %[b0], %[carry], %[scalar], %[a0]   \n\t"   \
            "umaal    %[b1], %[carry], %[scalar], %[a1]   \n\t"   \
            "str      %[b0], [%[acc], #-8]                \n\t"   \
            "str      %[b1], [%[acc], #-4]                \n\t"

#define MULADDC_X2_STOP                                      \
            : [a0]     "=&r" (tmp_a0),                       \
              [b0]     "=&r" (tmp_b0),                       \
              [a1]     "=&r" (tmp_a1),                       \
              [b1]     "=&r" (tmp_b1),                       \
              [in]     "+r"  (s),                            \
              [acc]    "+r"  (d),                            \
              [carry]  "+l"  (c)                             \
            : [scalar] "r"   (b)                             \
            : "memory"                                       \
        );                                                   \
    }

#else

#define MULADDC_X1_INIT                                 \
    asm(                                                \
            "ldr    r0, %3                      \n\t"   \
            "ldr    r1, %4                      \n\t"   \
            "ldr    r2, %5                      \n\t"   \
            "ldr    r3, %6                      \n\t"

#define MULADDC_X1_CORE                                 \
            "ldr    r4, [r0], #4                \n\t"   \
            "mov    r5, #0                      \n\t"   \
            "ldr    r6, [r1]                    \n\t"   \
            "umlal  r2, r5, r3, r4              \n\t"   \
            "adds   r4, r6, r2                  \n\t"   \
            "adc    r2, r5, #0                  \n\t"   \
            "str    r4, [r1], #4                \n\t"

#define MULADDC_X1_STOP                                 \
            "str    r2, %0                      \n\t"   \
            "str    r1, %1                      \n\t"   \
            "str    r0, %2                      \n\t"   \
         : "=m" (c),  "=m" (d), "=m" (s)        \
         : "m" (s), "m" (d), "m" (c), "m" (b)   \
         : "r0", "r1", "r2", "r3", "r4", "r5",  \
           "r6", "cc"                     \
         );

#endif /* ISA codepath selection */

#endif /* defined(__arm__) */

#if defined(__alpha__)

#define MULADDC_X1_INIT                 \
    asm(                                \
        "ldq    $1, %3          \n\t"   \
        "ldq    $2, %4          \n\t"   \
        "ldq    $3, %5          \n\t"   \
        "ldq    $4, %6          \n\t"

#define MULADDC_X1_CORE                 \
        "ldq    $6,  0($1)      \n\t"   \
        "addq   $1,  8, $1      \n\t"   \
        "mulq   $6, $4, $7      \n\t"   \
        "umulh  $6, $4, $6      \n\t"   \
        "addq   $7, $3, $7      \n\t"   \
        "cmpult $7, $3, $3      \n\t"   \
        "ldq    $5,  0($2)      \n\t"   \
        "addq   $7, $5, $7      \n\t"   \
        "cmpult $7, $5, $5      \n\t"   \
        "stq    $7,  0($2)      \n\t"   \
        "addq   $2,  8, $2      \n\t"   \
        "addq   $6, $3, $3      \n\t"   \
        "addq   $5, $3, $3      \n\t"

#define MULADDC_X1_STOP                 \
        "stq    $3, %0          \n\t"   \
        "stq    $2, %1          \n\t"   \
        "stq    $1, %2          \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)              \
        : "m" (s), "m" (d), "m" (c), "m" (b)        \
        : "$1", "$2", "$3", "$4", "$5", "$6", "$7"  \
    );
#endif /* Alpha */

#if defined(__mips__) && !defined(__mips64)

#define MULADDC_X1_INIT                 \
    asm(                                \
        "lw     $10, %3         \n\t"   \
        "lw     $11, %4         \n\t"   \
        "lw     $12, %5         \n\t"   \
        "lw     $13, %6         \n\t"

#define MULADDC_X1_CORE                 \
        "lw     $14, 0($10)     \n\t"   \
        "multu  $13, $14        \n\t"   \
        "addi   $10, $10, 4     \n\t"   \
        "mflo   $14             \n\t"   \
        "mfhi   $9              \n\t"   \
        "addu   $14, $12, $14   \n\t"   \
        "lw     $15, 0($11)     \n\t"   \
        "sltu   $12, $14, $12   \n\t"   \
        "addu   $15, $14, $15   \n\t"   \
        "sltu   $14, $15, $14   \n\t"   \
        "addu   $12, $12, $9    \n\t"   \
        "sw     $15, 0($11)     \n\t"   \
        "addu   $12, $12, $14   \n\t"   \
        "addi   $11, $11, 4     \n\t"

#define MULADDC_X1_STOP                 \
        "sw     $12, %0         \n\t"   \
        "sw     $11, %1         \n\t"   \
        "sw     $10, %2         \n\t"   \
        : "=m" (c), "=m" (d), "=m" (s)                      \
        : "m" (s), "m" (d), "m" (c), "m" (b)                \
        : "$9", "$10", "$11", "$12", "$13", "$14", "$15", "lo", "hi" \
    );

#endif /* MIPS */
#endif /* GNUC */

#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)

#define MULADDC_X1_INIT                         \
    __asm   mov     esi, s                      \
    __asm   mov     edi, d                      \
    __asm   mov     ecx, c                      \
    __asm   mov     ebx, b

#define MULADDC_X1_CORE                         \
    __asm   lodsd                               \
    __asm   mul     ebx                         \
    __asm   add     eax, ecx                    \
    __asm   adc     edx, 0                      \
    __asm   add     eax, [edi]                  \
    __asm   adc     edx, 0                      \
    __asm   mov     ecx, edx                    \
    __asm   stosd

#define MULADDC_X1_STOP                         \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi

#if defined(MBEDTLS_HAVE_SSE2)

#define EMIT __asm _emit

#define MULADDC_X8_INIT MULADDC_X1_INIT

#define MULADDC_X8_CORE                         \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC9             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC3             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x1F             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x16             \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x7E  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xEE             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x67  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xFC             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x0F             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x56  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5E  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCD             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xD5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCF             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xE5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xF5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDD             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCE             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x1C  \
    EMIT 0x83  EMIT 0xC7  EMIT 0x20             \
    EMIT 0x83  EMIT 0xC6  EMIT 0x20             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x7E  EMIT 0xC9

#define MULADDC_X8_STOP                         \
    EMIT 0x0F  EMIT 0x77                        \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi

#endif /* SSE2 */
#endif /* MSVC */

#if !defined(MULADDC_X1_CORE)
#if defined(MBEDTLS_HAVE_UDBL)

#define MULADDC_X1_INIT                 \
{                                       \
    mbedtls_t_udbl r;                           \
    mbedtls_mpi_uint r0, r1;

#define MULADDC_X1_CORE                 \
    r   = *(s++) * (mbedtls_t_udbl) b;          \
    r0  = (mbedtls_mpi_uint) r;                   \
    r1  = (mbedtls_mpi_uint)( r >> biL );         \
    r0 += c;  r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_X1_STOP                 \
}

#else

#define MULADDC_X1_INIT                 \
{                                       \
    mbedtls_mpi_uint s0, s1, b0, b1;              \
    mbedtls_mpi_uint r0, r1, rx, ry;              \
    b0 = ( b << biH ) >> biH;           \
    b1 = ( b >> biH );

#define MULADDC_X1_CORE                 \
    s0 = ( *s << biH ) >> biH;          \
    s1 = ( *s >> biH ); s++;            \
    rx = s0 * b1; r0 = s0 * b0;         \
    ry = s1 * b0; r1 = s1 * b1;         \
    r1 += ( rx >> biH );                \
    r1 += ( ry >> biH );                \
    rx <<= biH; ry <<= biH;             \
    r0 += rx; r1 += (r0 < rx);          \
    r0 += ry; r1 += (r0 < ry);          \
    r0 +=  c; r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_X1_STOP                 \
}

#endif /* C (longlong) */
#endif /* C (generic)  */

#if !defined(MULADDC_X2_CORE)
#define MULADDC_X2_INIT MULADDC_X1_INIT
#define MULADDC_X2_STOP MULADDC_X1_STOP
#define MULADDC_X2_CORE MULADDC_X1_CORE MULADDC_X1_CORE
#endif /* MULADDC_X2_CORE */

#if !defined(MULADDC_X4_CORE)
#define MULADDC_X4_INIT MULADDC_X2_INIT
#define MULADDC_X4_STOP MULADDC_X2_STOP
#define MULADDC_X4_CORE MULADDC_X2_CORE MULADDC_X2_CORE
#endif /* MULADDC_X4_CORE */

#if !defined(MULADDC_X8_CORE)
#define MULADDC_X8_INIT MULADDC_X4_INIT
#define MULADDC_X8_STOP MULADDC_X4_STOP
#define MULADDC_X8_CORE MULADDC_X4_CORE MULADDC_X4_CORE
#endif /* MULADDC_X8_CORE */

/* *INDENT-ON* */
#endif /* bn_mul.h */

/* ===== Retained cipher.h ===== */

/**
 * \file cipher.h
 *
 * \brief This file contains an abstraction interface for use with the cipher
 * primitives provided by the library. It provides a common interface to all of
 * the available cipher operations.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/cipher.h.
 */

#ifndef MBEDTLS_CIPHER_H
#define MBEDTLS_CIPHER_H


#include <stddef.h>

#define MBEDTLS_CIPHER_MODE_AEAD

/** The selected feature is not available. */
#define MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE  -0x6080
/** Bad input parameters. */
#define MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA       -0x6100
/** Failed to allocate memory. */
#define MBEDTLS_ERR_CIPHER_ALLOC_FAILED         -0x6180
/** Input data contains invalid padding and is rejected. */
#define MBEDTLS_ERR_CIPHER_INVALID_PADDING      -0x6200
/** Decryption of block requires a full block. */
#define MBEDTLS_ERR_CIPHER_FULL_BLOCK_EXPECTED  -0x6280
/** Authentication failed (for AEAD modes). */
#define MBEDTLS_ERR_CIPHER_AUTH_FAILED          -0x6300
/** The context is invalid. For example, because it was freed. */
#define MBEDTLS_ERR_CIPHER_INVALID_CONTEXT      -0x6380

#define MBEDTLS_CIPHER_VARIABLE_IV_LEN     0x01    /**< Cipher accepts IVs of variable length. */
#define MBEDTLS_CIPHER_VARIABLE_KEY_LEN    0x02    /**< Cipher accepts keys of variable length. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief     Supported cipher types.
 *
 * \warning   DES/3DES are considered weak ciphers and their use
 *            constitutes a security risk. We recommend considering stronger
 *            ciphers instead.
 */
typedef enum {
    MBEDTLS_CIPHER_ID_NONE = 0,  /**< Placeholder to mark the end of cipher ID lists. */
    MBEDTLS_CIPHER_ID_NULL,      /**< The identity cipher, treated as a stream cipher. */
    MBEDTLS_CIPHER_ID_AES,       /**< The AES cipher. */
    MBEDTLS_CIPHER_ID_DES,       /**< The DES cipher. \warning DES is considered weak. */
    MBEDTLS_CIPHER_ID_3DES,      /**< The Triple DES cipher. \warning 3DES is considered weak. */
    MBEDTLS_CIPHER_ID_CAMELLIA,  /**< The Camellia cipher. */
    MBEDTLS_CIPHER_ID_ARIA,      /**< The Aria cipher. */
    MBEDTLS_CIPHER_ID_CHACHA20,  /**< The ChaCha20 cipher. */
} mbedtls_cipher_id_t;

/**
 * \brief     Supported {cipher type, cipher mode} pairs.
 *
 * \warning   DES/3DES are considered weak ciphers and their use
 *            constitutes a security risk. We recommend considering stronger
 *            ciphers instead.
 */
typedef enum {
    MBEDTLS_CIPHER_NONE = 0,             /**< Placeholder to mark the end of cipher-pair lists. */
    MBEDTLS_CIPHER_NULL,                 /**< The identity stream cipher. */
    MBEDTLS_CIPHER_AES_128_ECB,          /**< AES cipher with 128-bit ECB mode. */
    MBEDTLS_CIPHER_AES_192_ECB,          /**< AES cipher with 192-bit ECB mode. */
    MBEDTLS_CIPHER_AES_256_ECB,          /**< AES cipher with 256-bit ECB mode. */
    MBEDTLS_CIPHER_AES_128_CBC,          /**< AES cipher with 128-bit CBC mode. */
    MBEDTLS_CIPHER_AES_192_CBC,          /**< AES cipher with 192-bit CBC mode. */
    MBEDTLS_CIPHER_AES_256_CBC,          /**< AES cipher with 256-bit CBC mode. */
    MBEDTLS_CIPHER_AES_128_CFB128,       /**< AES cipher with 128-bit CFB128 mode. */
    MBEDTLS_CIPHER_AES_192_CFB128,       /**< AES cipher with 192-bit CFB128 mode. */
    MBEDTLS_CIPHER_AES_256_CFB128,       /**< AES cipher with 256-bit CFB128 mode. */
    MBEDTLS_CIPHER_AES_128_CTR,          /**< AES cipher with 128-bit CTR mode. */
    MBEDTLS_CIPHER_AES_192_CTR,          /**< AES cipher with 192-bit CTR mode. */
    MBEDTLS_CIPHER_AES_256_CTR,          /**< AES cipher with 256-bit CTR mode. */
    MBEDTLS_CIPHER_AES_128_GCM,          /**< AES cipher with 128-bit GCM mode. */
    MBEDTLS_CIPHER_AES_192_GCM,          /**< AES cipher with 192-bit GCM mode. */
    MBEDTLS_CIPHER_AES_256_GCM,          /**< AES cipher with 256-bit GCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_ECB,     /**< Camellia cipher with 128-bit ECB mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_ECB,     /**< Camellia cipher with 192-bit ECB mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_ECB,     /**< Camellia cipher with 256-bit ECB mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_CBC,     /**< Camellia cipher with 128-bit CBC mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_CBC,     /**< Camellia cipher with 192-bit CBC mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_CBC,     /**< Camellia cipher with 256-bit CBC mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_CFB128,  /**< Camellia cipher with 128-bit CFB128 mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_CFB128,  /**< Camellia cipher with 192-bit CFB128 mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_CFB128,  /**< Camellia cipher with 256-bit CFB128 mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_CTR,     /**< Camellia cipher with 128-bit CTR mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_CTR,     /**< Camellia cipher with 192-bit CTR mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_CTR,     /**< Camellia cipher with 256-bit CTR mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_GCM,     /**< Camellia cipher with 128-bit GCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_GCM,     /**< Camellia cipher with 192-bit GCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_GCM,     /**< Camellia cipher with 256-bit GCM mode. */
    MBEDTLS_CIPHER_DES_ECB,              /**< DES cipher with ECB mode. \warning DES is considered weak. */
    MBEDTLS_CIPHER_DES_CBC,              /**< DES cipher with CBC mode. \warning DES is considered weak. */
    MBEDTLS_CIPHER_DES_EDE_ECB,          /**< DES cipher with EDE ECB mode. \warning 3DES is considered weak. */
    MBEDTLS_CIPHER_DES_EDE_CBC,          /**< DES cipher with EDE CBC mode. \warning 3DES is considered weak. */
    MBEDTLS_CIPHER_DES_EDE3_ECB,         /**< DES cipher with EDE3 ECB mode. \warning 3DES is considered weak. */
    MBEDTLS_CIPHER_DES_EDE3_CBC,         /**< DES cipher with EDE3 CBC mode. \warning 3DES is considered weak. */
    MBEDTLS_CIPHER_AES_128_CCM,          /**< AES cipher with 128-bit CCM mode. */
    MBEDTLS_CIPHER_AES_192_CCM,          /**< AES cipher with 192-bit CCM mode. */
    MBEDTLS_CIPHER_AES_256_CCM,          /**< AES cipher with 256-bit CCM mode. */
    MBEDTLS_CIPHER_AES_128_CCM_STAR_NO_TAG, /**< AES cipher with 128-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_AES_192_CCM_STAR_NO_TAG, /**< AES cipher with 192-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_AES_256_CCM_STAR_NO_TAG, /**< AES cipher with 256-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_CCM,     /**< Camellia cipher with 128-bit CCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_CCM,     /**< Camellia cipher with 192-bit CCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_CCM,     /**< Camellia cipher with 256-bit CCM mode. */
    MBEDTLS_CIPHER_CAMELLIA_128_CCM_STAR_NO_TAG, /**< Camellia cipher with 128-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_CAMELLIA_192_CCM_STAR_NO_TAG, /**< Camellia cipher with 192-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_CAMELLIA_256_CCM_STAR_NO_TAG, /**< Camellia cipher with 256-bit CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_ARIA_128_ECB,         /**< Aria cipher with 128-bit key and ECB mode. */
    MBEDTLS_CIPHER_ARIA_192_ECB,         /**< Aria cipher with 192-bit key and ECB mode. */
    MBEDTLS_CIPHER_ARIA_256_ECB,         /**< Aria cipher with 256-bit key and ECB mode. */
    MBEDTLS_CIPHER_ARIA_128_CBC,         /**< Aria cipher with 128-bit key and CBC mode. */
    MBEDTLS_CIPHER_ARIA_192_CBC,         /**< Aria cipher with 192-bit key and CBC mode. */
    MBEDTLS_CIPHER_ARIA_256_CBC,         /**< Aria cipher with 256-bit key and CBC mode. */
    MBEDTLS_CIPHER_ARIA_128_CFB128,      /**< Aria cipher with 128-bit key and CFB-128 mode. */
    MBEDTLS_CIPHER_ARIA_192_CFB128,      /**< Aria cipher with 192-bit key and CFB-128 mode. */
    MBEDTLS_CIPHER_ARIA_256_CFB128,      /**< Aria cipher with 256-bit key and CFB-128 mode. */
    MBEDTLS_CIPHER_ARIA_128_CTR,         /**< Aria cipher with 128-bit key and CTR mode. */
    MBEDTLS_CIPHER_ARIA_192_CTR,         /**< Aria cipher with 192-bit key and CTR mode. */
    MBEDTLS_CIPHER_ARIA_256_CTR,         /**< Aria cipher with 256-bit key and CTR mode. */
    MBEDTLS_CIPHER_ARIA_128_GCM,         /**< Aria cipher with 128-bit key and GCM mode. */
    MBEDTLS_CIPHER_ARIA_192_GCM,         /**< Aria cipher with 192-bit key and GCM mode. */
    MBEDTLS_CIPHER_ARIA_256_GCM,         /**< Aria cipher with 256-bit key and GCM mode. */
    MBEDTLS_CIPHER_ARIA_128_CCM,         /**< Aria cipher with 128-bit key and CCM mode. */
    MBEDTLS_CIPHER_ARIA_192_CCM,         /**< Aria cipher with 192-bit key and CCM mode. */
    MBEDTLS_CIPHER_ARIA_256_CCM,         /**< Aria cipher with 256-bit key and CCM mode. */
    MBEDTLS_CIPHER_ARIA_128_CCM_STAR_NO_TAG, /**< Aria cipher with 128-bit key and CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_ARIA_192_CCM_STAR_NO_TAG, /**< Aria cipher with 192-bit key and CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_ARIA_256_CCM_STAR_NO_TAG, /**< Aria cipher with 256-bit key and CCM_STAR_NO_TAG mode. */
    MBEDTLS_CIPHER_AES_128_OFB,          /**< AES 128-bit cipher in OFB mode. */
    MBEDTLS_CIPHER_AES_192_OFB,          /**< AES 192-bit cipher in OFB mode. */
    MBEDTLS_CIPHER_AES_256_OFB,          /**< AES 256-bit cipher in OFB mode. */
    MBEDTLS_CIPHER_AES_128_XTS,          /**< AES 128-bit cipher in XTS block mode. */
    MBEDTLS_CIPHER_AES_256_XTS,          /**< AES 256-bit cipher in XTS block mode. */
    MBEDTLS_CIPHER_CHACHA20,             /**< ChaCha20 stream cipher. */
    MBEDTLS_CIPHER_CHACHA20_POLY1305,    /**< ChaCha20-Poly1305 AEAD cipher. */
    MBEDTLS_CIPHER_AES_128_KW,           /**< AES cipher with 128-bit NIST KW mode. */
    MBEDTLS_CIPHER_AES_192_KW,           /**< AES cipher with 192-bit NIST KW mode. */
    MBEDTLS_CIPHER_AES_256_KW,           /**< AES cipher with 256-bit NIST KW mode. */
    MBEDTLS_CIPHER_AES_128_KWP,          /**< AES cipher with 128-bit NIST KWP mode. */
    MBEDTLS_CIPHER_AES_192_KWP,          /**< AES cipher with 192-bit NIST KWP mode. */
    MBEDTLS_CIPHER_AES_256_KWP,          /**< AES cipher with 256-bit NIST KWP mode. */
} mbedtls_cipher_type_t;

/** Supported cipher modes. */
typedef enum {
    MBEDTLS_MODE_NONE = 0,               /**< None.                        */
    MBEDTLS_MODE_ECB,                    /**< The ECB cipher mode.         */
    MBEDTLS_MODE_CBC,                    /**< The CBC cipher mode.         */
    MBEDTLS_MODE_CFB,                    /**< The CFB cipher mode.         */
    MBEDTLS_MODE_OFB,                    /**< The OFB cipher mode.         */
    MBEDTLS_MODE_CTR,                    /**< The CTR cipher mode.         */
    MBEDTLS_MODE_GCM,                    /**< The GCM cipher mode.         */
    MBEDTLS_MODE_STREAM,                 /**< The stream cipher mode.      */
    MBEDTLS_MODE_CCM,                    /**< The CCM cipher mode.         */
    MBEDTLS_MODE_CCM_STAR_NO_TAG,        /**< The CCM*-no-tag cipher mode. */
    MBEDTLS_MODE_XTS,                    /**< The XTS cipher mode.         */
    MBEDTLS_MODE_CHACHAPOLY,             /**< The ChaCha-Poly cipher mode. */
    MBEDTLS_MODE_KW,                     /**< The SP800-38F KW mode */
    MBEDTLS_MODE_KWP,                    /**< The SP800-38F KWP mode */
} mbedtls_cipher_mode_t;

/** Supported cipher padding types. */
typedef enum {
    MBEDTLS_PADDING_PKCS7 = 0,     /**< PKCS7 padding (default).        */
    MBEDTLS_PADDING_ONE_AND_ZEROS, /**< ISO/IEC 7816-4 padding.         */
    MBEDTLS_PADDING_ZEROS_AND_LEN, /**< ANSI X.923 padding.             */
    MBEDTLS_PADDING_ZEROS,         /**< Zero padding (not reversible). */
    MBEDTLS_PADDING_NONE,          /**< Never pad (full blocks only).   */
} mbedtls_cipher_padding_t;

/** Type of operation. */
typedef enum {
    MBEDTLS_OPERATION_NONE = -1,
    MBEDTLS_DECRYPT = 0,
    MBEDTLS_ENCRYPT,
} mbedtls_operation_t;

enum {
    /** Undefined key length. */
    MBEDTLS_KEY_LENGTH_NONE = 0,
    /** Key length, in bits (including parity), for DES keys. \warning DES is considered weak. */
    MBEDTLS_KEY_LENGTH_DES  = 64,
    /** Key length in bits, including parity, for DES in two-key EDE. \warning 3DES is considered weak. */
    MBEDTLS_KEY_LENGTH_DES_EDE = 128,
    /** Key length in bits, including parity, for DES in three-key EDE. \warning 3DES is considered weak. */
    MBEDTLS_KEY_LENGTH_DES_EDE3 = 192,
};

/** Maximum length of any IV, in Bytes. */
/* This should ideally be derived automatically from list of ciphers.
 * This should be kept in sync with MBEDTLS_SSL_MAX_IV_LENGTH defined
 * in library/ssl_misc.h. */
#define MBEDTLS_MAX_IV_LENGTH      16

/** Maximum block size of any cipher, in Bytes. */
/* This should ideally be derived automatically from list of ciphers.
 * This should be kept in sync with MBEDTLS_SSL_MAX_BLOCK_LENGTH defined
 * in library/ssl_misc.h. */
#define MBEDTLS_MAX_BLOCK_LENGTH   16

/** Maximum key length, in Bytes. */
/* This should ideally be derived automatically from list of ciphers.
 * For now, only check whether XTS is enabled which uses 64 Byte keys,
 * and use 32 Bytes as an upper bound for the maximum key length otherwise.
 * This should be kept in sync with MBEDTLS_SSL_MAX_BLOCK_LENGTH defined
 * in library/ssl_misc.h, which however deliberately ignores the case of XTS
 * since the latter isn't used in SSL/TLS. */
#define MBEDTLS_MAX_KEY_LENGTH     32

/**
 * Base cipher information (opaque struct).
 */
typedef struct mbedtls_cipher_base_t mbedtls_cipher_base_t;

/**
 * CMAC context (opaque struct).
 */
typedef struct mbedtls_cmac_context_t mbedtls_cmac_context_t;

/**
 * Cipher information. Allows calling cipher functions
 * in a generic way.
 *
 * \note        The library does not support custom cipher info structures,
 *              only built-in structures returned by the functions
 *              mbedtls_cipher_info_from_string(),
 *              mbedtls_cipher_info_from_type(),
 *              mbedtls_cipher_info_from_values(),
 *              mbedtls_cipher_info_from_psa().
 *
 * \note        Some fields store a value that has been right-shifted to save
 *              code-size, so should not be used directly. The accessor
 *              functions adjust for this and return the "natural" value.
 */
typedef struct mbedtls_cipher_info_t {
    /** Name of the cipher. */
    const char *MBEDTLS_PRIVATE(name);

    /** The block size, in bytes. */
    unsigned int MBEDTLS_PRIVATE(block_size) : 5;

    /** IV or nonce size, in bytes (right shifted by #MBEDTLS_IV_SIZE_SHIFT).
     * For ciphers that accept variable IV sizes,
     * this is the recommended size.
     */
    unsigned int MBEDTLS_PRIVATE(iv_size) : 3;

    /** The cipher key length, in bits (right shifted by #MBEDTLS_KEY_BITLEN_SHIFT).
     * This is the default length for variable sized ciphers.
     * Includes parity bits for ciphers like DES.
     */
    unsigned int MBEDTLS_PRIVATE(key_bitlen) : 4;

    /** The cipher mode (as per mbedtls_cipher_mode_t).
     * For example, MBEDTLS_MODE_CBC.
     */
    unsigned int MBEDTLS_PRIVATE(mode) : 4;

    /** Full cipher identifier (as per mbedtls_cipher_type_t).
     * For example, MBEDTLS_CIPHER_AES_256_CBC.
     *
     * This could be 7 bits, but 8 bits retains byte alignment for the
     * next field, which reduces code size to access that field.
     */
    unsigned int MBEDTLS_PRIVATE(type) : 8;

    /** Bitflag comprised of MBEDTLS_CIPHER_VARIABLE_IV_LEN and
     *  MBEDTLS_CIPHER_VARIABLE_KEY_LEN indicating whether the
     *  cipher supports variable IV or variable key sizes, respectively.
     */
    unsigned int MBEDTLS_PRIVATE(flags) : 2;

    /** Index to LUT for base cipher information and functions. */
    unsigned int MBEDTLS_PRIVATE(base_idx) : 5;

} mbedtls_cipher_info_t;

/* For internal use only.
 * These are used to more compactly represent the fields above. */
#define MBEDTLS_KEY_BITLEN_SHIFT  6
#define MBEDTLS_IV_SIZE_SHIFT     2
/**
 * Generic cipher context.
 */
typedef struct mbedtls_cipher_context_t {
    /** Information about the associated cipher. */
    const mbedtls_cipher_info_t *MBEDTLS_PRIVATE(cipher_info);

    /** Key length to use. */
    int MBEDTLS_PRIVATE(key_bitlen);

    /** Operation that the key of the context has been
     * initialized for.
     */
    mbedtls_operation_t MBEDTLS_PRIVATE(operation);

#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)
    /** Padding functions to use, if relevant for
     * the specific cipher mode.
     */
    void(*MBEDTLS_PRIVATE(add_padding))(unsigned char *output, size_t olen,
                                        size_t data_len);
    /* Report invalid-padding condition through the output parameter
     * invalid_padding. To minimize changes in Mbed TLS 3.6, where this
     * declaration is in a public header, use the public type size_t
     * rather than the internal type mbedtls_ct_condition_t. */
    int(*MBEDTLS_PRIVATE(get_padding))(unsigned char *input, size_t ilen,
                                       size_t *data_len,
                                       size_t *invalid_padding, MinTlsDiagnostics* diagnostics);
#endif

    /** Buffer for input that has not been processed yet. */
    unsigned char MBEDTLS_PRIVATE(unprocessed_data)[MBEDTLS_MAX_BLOCK_LENGTH];

    /** Number of Bytes that have not been processed yet. */
    size_t MBEDTLS_PRIVATE(unprocessed_len);

    /** Current IV or NONCE_COUNTER for CTR-mode, data unit (or sector) number
     * for XTS-mode. */
    unsigned char MBEDTLS_PRIVATE(iv)[MBEDTLS_MAX_IV_LENGTH];

    /** IV size in Bytes, for ciphers with variable-length IVs. */
    size_t MBEDTLS_PRIVATE(iv_size);

    /** The cipher-specific context. */
    void *MBEDTLS_PRIVATE(cipher_ctx);

} mbedtls_cipher_context_t;

/**
 * \brief               This function retrieves the cipher-information
 *                      structure associated with the given cipher type.
 *
 * \param cipher_type   Type of the cipher to search for.
 *
 * \return              The cipher information structure associated with the
 *                      given \p cipher_type.
 * \return              \c NULL if the associated cipher information is not found.
 */
const mbedtls_cipher_info_t *mbedtls_cipher_info_from_type(const mbedtls_cipher_type_t cipher_type);

/**
 * \brief               This function retrieves the cipher-information
 *                      structure associated with the given cipher ID,
 *                      key size and mode.
 *
 * \param cipher_id     The ID of the cipher to search for. For example,
 *                      #MBEDTLS_CIPHER_ID_AES.
 * \param key_bitlen    The length of the key in bits.
 * \param mode          The cipher mode. For example, #MBEDTLS_MODE_CBC.
 *
 * \return              The cipher information structure associated with the
 *                      given \p cipher_id.
 * \return              \c NULL if the associated cipher information is not found.
 */
const mbedtls_cipher_info_t *mbedtls_cipher_info_from_values(const mbedtls_cipher_id_t cipher_id,
                                                             int key_bitlen,
                                                             const mbedtls_cipher_mode_t mode);

/**
 * \brief               Retrieve the operation mode for a cipher info structure.
 *
 * \param[in] info      The cipher info structure to query.
 *                      This may be \c NULL.
 *
 * \return              The cipher mode (\c MBEDTLS_MODE_xxx).
 * \return              #MBEDTLS_MODE_NONE if \p info is \c NULL.
 */
static inline mbedtls_cipher_mode_t mbedtls_cipher_info_get_mode(
    const mbedtls_cipher_info_t *info)
{
    if (info == NULL) {
        return MBEDTLS_MODE_NONE;
    } else {
        return (mbedtls_cipher_mode_t) info->MBEDTLS_PRIVATE(mode);
    }
}

/**
 * \brief               Retrieve the key size for a cipher info structure.
 *
 * \param[in] info      The cipher info structure to query.
 *                      This may be \c NULL.
 *
 * \return              The key length in bits.
 *                      For variable-sized ciphers, this is the default length.
 *                      For DES, this includes the parity bits.
 * \return              \c 0 if \p info is \c NULL.
 */
static inline size_t mbedtls_cipher_info_get_key_bitlen(
    const mbedtls_cipher_info_t *info)
{
    if (info == NULL) {
        return 0;
    } else {
        return ((size_t) info->MBEDTLS_PRIVATE(key_bitlen)) << MBEDTLS_KEY_BITLEN_SHIFT;
    }
}

/**
 * \brief        This function returns the block size of the given
 *               cipher info structure in bytes.
 *
 * \param info   The cipher info structure. This may be \c NULL.
 *
 * \return       The block size of the cipher.
 * \return       \c 1 if the cipher is a stream cipher.
 * \return       \c 0 if \p info is \c NULL.
 */
static inline size_t mbedtls_cipher_info_get_block_size(
    const mbedtls_cipher_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    return (size_t) (info->MBEDTLS_PRIVATE(block_size));
}

/**
 * \brief               This function initializes a \p ctx as NONE.
 *
 * \param ctx           The context to be initialized. This must not be \c NULL.
 */
void mbedtls_cipher_init(mbedtls_cipher_context_t *ctx);

/**
 * \brief               This function frees and clears the cipher-specific
 *                      context of \p ctx. Freeing \p ctx itself remains the
 *                      responsibility of the caller.
 *
 * \param ctx           The context to be freed. If this is \c NULL, the
 *                      function has no effect, otherwise this must point to an
 *                      initialized context.
 */
void mbedtls_cipher_free(mbedtls_cipher_context_t *ctx);

/**
 * \brief               This function prepares a cipher context for
 *                      use with the given cipher primitive.
 *
 * \note                After calling this function, you should call
 *                      mbedtls_cipher_setkey() and, if the mode uses padding,
 *                      mbedtls_cipher_set_padding_mode(), then for each
 *                      message to encrypt or decrypt with this key, either:
 *                      - mbedtls_cipher_crypt() for one-shot processing with
 *                      non-AEAD modes;
 *                      - mbedtls_cipher_auth_encrypt_ext() or
 *                      mbedtls_cipher_auth_decrypt_ext() for one-shot
 *                      processing with AEAD modes or NIST_KW;
 *                      - for multi-part processing, see the documentation of
 *                      mbedtls_cipher_reset().
 *
 * \param ctx           The context to prepare. This must be initialized by
 *                      a call to mbedtls_cipher_init() first.
 * \param cipher_info   The cipher to use.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA on
 *                      parameter-verification failure.
 * \return              #MBEDTLS_ERR_CIPHER_ALLOC_FAILED if allocation of the
 *                      cipher-specific context fails.
 */
int mbedtls_cipher_setup(mbedtls_cipher_context_t *ctx,
                         const mbedtls_cipher_info_t *cipher_info, MinTlsDiagnostics* diagnostics);

/**
 * \brief        This function returns the block size of the given cipher
 *               in bytes.
 *
 * \param ctx    The context of the cipher.
 *
 * \return       The block size of the underlying cipher.
 * \return       \c 1 if the cipher is a stream cipher.
 * \return       \c 0 if \p ctx has not been initialized.
 */
static inline unsigned int mbedtls_cipher_get_block_size(
    const mbedtls_cipher_context_t *ctx)
{
    if (ctx->MBEDTLS_PRIVATE(cipher_info) == NULL) {
        return 0;
    }

    return (unsigned int) ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(block_size);
}

/**
 * \brief        This function returns the mode of operation for
 *               the cipher. For example, MBEDTLS_MODE_CBC.
 *
 * \param ctx    The context of the cipher. This must be initialized.
 *
 * \return       The mode of operation.
 * \return       #MBEDTLS_MODE_NONE if \p ctx has not been initialized.
 */
static inline mbedtls_cipher_mode_t mbedtls_cipher_get_cipher_mode(
    const mbedtls_cipher_context_t *ctx)
{
    if (ctx->MBEDTLS_PRIVATE(cipher_info) == NULL) {
        return MBEDTLS_MODE_NONE;
    }

    return (mbedtls_cipher_mode_t) ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(mode);
}

/**
 * \brief               This function sets the key to use with the given context.
 *
 * \param ctx           The generic cipher context. This must be initialized and
 *                      bound to a cipher information structure.
 * \param key           The key to use. This must be a readable buffer of at
 *                      least \p key_bitlen Bits.
 * \param key_bitlen    The key length to use, in Bits.
 * \param operation     The operation that the key will be used for:
 *                      #MBEDTLS_ENCRYPT or #MBEDTLS_DECRYPT.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA on
 *                      parameter-verification failure.
 * \return              A cipher-specific error code on failure.
 */
int mbedtls_cipher_setkey(mbedtls_cipher_context_t *ctx,
                          const unsigned char *key,
                          int key_bitlen,
                          const mbedtls_operation_t operation, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)

#endif

/**
 * \brief               The generic cipher update function. It encrypts or
 *                      decrypts using the given cipher context. Writes as
 *                      many block-sized blocks of data as possible to output.
 *                      Any data that cannot be written immediately is either
 *                      added to the next block, or flushed when
 *                      mbedtls_cipher_finish() or mbedtls_cipher_finish_padded()
 *                      is called.
 *                      Exception: For MBEDTLS_MODE_ECB, expects a single block
 *                      in size. For example, 16 Bytes for AES.
 *
 * \param ctx           The generic cipher context. This must be initialized and
 *                      bound to a key.
 * \param input         The buffer holding the input data. This must be a
 *                      readable buffer of at least \p ilen Bytes.
 * \param ilen          The length of the input data.
 * \param output        The buffer for the output data. This must be able to
 *                      hold at least `ilen + block_size`. This must not be the
 *                      same buffer as \p input.
 * \param olen          The length of the output data, to be updated with the
 *                      actual number of Bytes written. This must not be
 *                      \c NULL.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA on
 *                      parameter-verification failure.
 * \return              #MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE on an
 *                      unsupported mode for a cipher.
 * \return              A cipher-specific error code on failure.
 */
int mbedtls_cipher_update(mbedtls_cipher_context_t *ctx,
                          const unsigned char *input,
                          size_t ilen, unsigned char *output,
                          size_t *olen, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_CIPHER_MODE_AEAD) || defined(MBEDTLS_NIST_KW_C)
/**
 * \brief               The authenticated encryption (AEAD/NIST_KW) function.
 *
 * \note                For AEAD modes, the tag will be appended to the
 *                      ciphertext, as recommended by RFC 5116.
 *                      (NIST_KW doesn't have a separate tag.)
 *
 * \param ctx           The generic cipher context. This must be initialized and
 *                      bound to a key, with an AEAD algorithm or NIST_KW.
 * \param iv            The nonce to use. This must be a readable buffer of
 *                      at least \p iv_len Bytes and may be \c NULL if \p
 *                      iv_len is \c 0.
 * \param iv_len        The length of the nonce. For AEAD ciphers, this must
 *                      satisfy the constraints imposed by the cipher used.
 *                      For NIST_KW, this must be \c 0.
 * \param ad            The additional data to authenticate. This must be a
 *                      readable buffer of at least \p ad_len Bytes, and may
 *                      be \c NULL is \p ad_len is \c 0.
 * \param ad_len        The length of \p ad. For NIST_KW, this must be \c 0.
 * \param input         The buffer holding the input data. This must be a
 *                      readable buffer of at least \p ilen Bytes, and may be
 *                      \c NULL if \p ilen is \c 0.
 * \param ilen          The length of the input data.
 * \param output        The buffer for the output data. This must be a
 *                      writable buffer of at least \p output_len Bytes, and
 *                      must not be \c NULL.
 * \param output_len    The length of the \p output buffer in Bytes. For AEAD
 *                      ciphers, this must be at least \p ilen + \p tag_len.
 *                      For NIST_KW, this must be at least \p ilen + 8
 *                      (rounded up to a multiple of 8 if KWP is used);
 *                      \p ilen + 15 is always a safe value.
 * \param olen          This will be filled with the actual number of Bytes
 *                      written to the \p output buffer. This must point to a
 *                      writable object of type \c size_t.
 * \param tag_len       The desired length of the authentication tag. For AEAD
 *                      ciphers, this must match the constraints imposed by
 *                      the cipher used, and in particular must not be \c 0.
 *                      For NIST_KW, this must be \c 0.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA on
 *                      parameter-verification failure.
 * \return              A cipher-specific error code on failure.
 */
int mbedtls_cipher_auth_encrypt_ext(mbedtls_cipher_context_t *ctx,
                                    const unsigned char *iv, size_t iv_len,
                                    const unsigned char *ad, size_t ad_len,
                                    const unsigned char *input, size_t ilen,
                                    unsigned char *output, size_t output_len,
                                    size_t *olen, size_t tag_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief               The authenticated encryption (AEAD/NIST_KW) function.
 *
 * \note                If the data is not authentic, then the output buffer
 *                      is zeroed out to prevent the unauthentic plaintext being
 *                      used, making this interface safer.
 *
 * \note                For AEAD modes, the tag must be appended to the
 *                      ciphertext, as recommended by RFC 5116.
 *                      (NIST_KW doesn't have a separate tag.)
 *
 * \param ctx           The generic cipher context. This must be initialized and
 *                      bound to a key, with an AEAD algorithm or NIST_KW.
 * \param iv            The nonce to use. This must be a readable buffer of
 *                      at least \p iv_len Bytes and may be \c NULL if \p
 *                      iv_len is \c 0.
 * \param iv_len        The length of the nonce. For AEAD ciphers, this must
 *                      satisfy the constraints imposed by the cipher used.
 *                      For NIST_KW, this must be \c 0.
 * \param ad            The additional data to authenticate. This must be a
 *                      readable buffer of at least \p ad_len Bytes, and may
 *                      be \c NULL is \p ad_len is \c 0.
 * \param ad_len        The length of \p ad. For NIST_KW, this must be \c 0.
 * \param input         The buffer holding the input data. This must be a
 *                      readable buffer of at least \p ilen Bytes, and may be
 *                      \c NULL if \p ilen is \c 0.
 * \param ilen          The length of the input data. For AEAD ciphers this
 *                      must be at least \p tag_len. For NIST_KW this must be
 *                      at least \c 8.
 * \param output        The buffer for the output data. This must be a
 *                      writable buffer of at least \p output_len Bytes, and
 *                      may be \c NULL if \p output_len is \c 0.
 * \param output_len    The length of the \p output buffer in Bytes. For AEAD
 *                      ciphers, this must be at least \p ilen - \p tag_len.
 *                      For NIST_KW, this must be at least \p ilen - 8.
 * \param olen          This will be filled with the actual number of Bytes
 *                      written to the \p output buffer. This must point to a
 *                      writable object of type \c size_t.
 * \param tag_len       The actual length of the authentication tag. For AEAD
 *                      ciphers, this must match the constraints imposed by
 *                      the cipher used, and in particular must not be \c 0.
 *                      For NIST_KW, this must be \c 0.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA on
 *                      parameter-verification failure.
 * \return              #MBEDTLS_ERR_CIPHER_AUTH_FAILED if data is not authentic.
 * \return              A cipher-specific error code on failure.
 */
int mbedtls_cipher_auth_decrypt_ext(mbedtls_cipher_context_t *ctx,
                                    const unsigned char *iv, size_t iv_len,
                                    const unsigned char *ad, size_t ad_len,
                                    const unsigned char *input, size_t ilen,
                                    unsigned char *output, size_t output_len,
                                    size_t *olen, size_t tag_len, MinTlsDiagnostics* diagnostics);
#endif /* MBEDTLS_CIPHER_MODE_AEAD || MBEDTLS_NIST_KW_C */
#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CIPHER_H */

/* ===== Retained cipher_invasive.h ===== */

/**
 * \file cipher_invasive.h
 *
 * \brief Cipher module: interfaces for invasive testing only.
 *
 * The interfaces in this file are intended for testing purposes only.
 * They SHOULD NOT be made available in library integrations except when
 * building the library for testing.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/cipher_invasive.h.
 */
#ifndef MBEDTLS_CIPHER_INVASIVE_H
#define MBEDTLS_CIPHER_INVASIVE_H


#endif /* MBEDTLS_CIPHER_INVASIVE_H */

/* ===== Retained cipher_wrap.h ===== */

/**
 * \file cipher_wrap.h
 *
 * \brief Cipher wrappers.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/cipher_wrap.h.
 */
#ifndef MBEDTLS_CIPHER_WRAP_H
#define MBEDTLS_CIPHER_WRAP_H



#ifdef __cplusplus
extern "C" {
#endif

/* Support for GCM either through Mbed TLS SW implementation or PSA */
#define MBEDTLS_CIPHER_HAVE_GCM_VIA_LEGACY_OR_USE_PSA

#define MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA

#if defined(MBEDTLS_CIPHER_HAVE_GCM_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CCM_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CCM_STAR_NO_TAG_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CHACHAPOLY_VIA_LEGACY_OR_USE_PSA)
#define MBEDTLS_CIPHER_HAVE_SOME_AEAD_VIA_LEGACY_OR_USE_PSA
#endif

/**
 * Base cipher information. The non-mode specific functions and values.
 */
struct mbedtls_cipher_base_t {
    /** Base Cipher type (e.g. MBEDTLS_CIPHER_ID_AES) */
    mbedtls_cipher_id_t cipher;

    /** Encrypt using ECB */
    int (*ecb_func)(void *ctx, mbedtls_operation_t mode,
                    const unsigned char *input, unsigned char *output, MinTlsDiagnostics* diagnostics);

    /** Set key for encryption purposes */
    int (*setkey_enc_func)(void *ctx, const unsigned char *key,
                           unsigned int key_bitlen, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    /** Set key for decryption purposes */
    int (*setkey_dec_func)(void *ctx, const unsigned char *key,
                           unsigned int key_bitlen, MinTlsDiagnostics* diagnostics);
#endif

    /** Allocate a new context */
    void * (*ctx_alloc_func)(void);

    /** Free the given context */
    void (*ctx_free_func)(void *ctx);

};

typedef struct {
    mbedtls_cipher_type_t type;
    const mbedtls_cipher_info_t *info;
} mbedtls_cipher_definition_t;

extern const mbedtls_cipher_definition_t mbedtls_cipher_definitions[];

extern int mbedtls_cipher_supported[];

extern const mbedtls_cipher_base_t * const mbedtls_cipher_base_lookup_table[];

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CIPHER_WRAP_H */

/* ===== Retained constant_time.h ===== */

/**
 *  Constant-time functions
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/constant_time.h.
 */

#ifndef MBEDTLS_CONSTANT_TIME_H
#define MBEDTLS_CONSTANT_TIME_H

#include <stddef.h>

/** Constant-time buffer comparison without branches.
 *
 * This is equivalent to the standard memcmp function, but is likely to be
 * compiled to code using bitwise operations rather than a branch, such that
 * the time taken is constant w.r.t. the data pointed to by \p a and \p b,
 * and w.r.t. whether \p a and \p b are equal or not. It is not constant-time
 * w.r.t. \p n .
 *
 * This function can be used to write constant-time code by replacing branches
 * with bit operations using masks.
 *
 * \param a     Pointer to the first buffer, containing at least \p n bytes. May not be NULL.
 * \param b     Pointer to the second buffer, containing at least \p n bytes. May not be NULL.
 * \param n     The number of bytes to compare.
 *
 * \return      Zero if the contents of the two buffers are the same,
 *              otherwise non-zero.
 */
int mbedtls_ct_memcmp(const void *a,
                      const void *b,
                      size_t n);

#endif /* MBEDTLS_CONSTANT_TIME_H */

/* ===== Retained ctr.h ===== */

/**
 * \file ctr.h
 *
 * \brief    This file contains common functionality for counter algorithms.
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ctr.h.
 */

#ifndef MBEDTLS_CTR_H
#define MBEDTLS_CTR_H


/**
 * \brief               Increment a big-endian 16-byte value.
 *                      This is quite performance-sensitive for AES-CTR and CTR-DRBG.
 *
 * \param n             A 16-byte value to be incremented.
 */
static inline void mbedtls_ctr_increment_counter(uint8_t n[16])
{
    // The 32-bit version seems to perform about the same as a 64-bit version
    // on 64-bit architectures, so no need to define a 64-bit version.
    for (int i = 3;; i--) {
        uint32_t x = MBEDTLS_GET_UINT32_BE(n, i << 2);
        x += 1;
        MBEDTLS_PUT_UINT32_BE(x, n, i << 2);
        if (x != 0 || i == 0) {
            break;
        }
    }
}

#endif /* MBEDTLS_CTR_H */

/* ===== Retained ctr_drbg.h ===== */

/**
 * \file ctr_drbg.h
 *
 * \brief    This file contains definitions and functions for the
 *           CTR_DRBG pseudorandom generator.
 *
 * CTR_DRBG is a standardized way of building a PRNG from a block-cipher
 * in counter mode operation, as defined in <em>NIST SP 800-90A:
 * Recommendation for Random Number Generation Using Deterministic Random
 * Bit Generators</em>.
 *
 * The Mbed TLS implementation of CTR_DRBG uses AES-256 (default) or AES-128
 * (if \c MBEDTLS_CTR_DRBG_USE_128_BIT_KEY is enabled at compile time)
 * as the underlying block cipher, with a derivation function.
 *
 * The security strength as defined in NIST SP 800-90A is
 * 128 bits when AES-128 is used (\c MBEDTLS_CTR_DRBG_USE_128_BIT_KEY enabled)
 * and 256 bits otherwise, provided that #MBEDTLS_CTR_DRBG_ENTROPY_LEN is
 * kept at its default value (and not overridden in mbedtls_config.h) and that the
 * DRBG instance is set up with default parameters.
 * See the documentation of mbedtls_ctr_drbg_seed() for more
 * information.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ctr_drbg.h.
 */

#ifndef MBEDTLS_CTR_DRBG_H
#define MBEDTLS_CTR_DRBG_H


/* The CTR_DRBG implementation can either directly call the low-level AES
 * module (gated by MBEDTLS_AES_C) or call the PSA API to perform AES
 * operations. Calling the AES module directly is the default, both for
 * maximum backward compatibility and because it's a bit more efficient
 * (less glue code).
 *
 * When MBEDTLS_AES_C is disabled, the CTR_DRBG module calls PSA crypto and
 * thus benefits from the PSA AES accelerator driver.
 * It is technically possible to enable MBEDTLS_CTR_DRBG_USE_PSA_CRYPTO
 * to use PSA even when MBEDTLS_AES_C is enabled, but there is very little
 * reason to do so other than testing purposes and this is not officially
 * supported.
 */


/* ===== Retained entropy.h ===== */

/**
 * \file entropy.h
 *
 * \brief Entropy accumulator implementation
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/entropy.h.
 */
#ifndef MBEDTLS_ENTROPY_H
#define MBEDTLS_ENTROPY_H


#include <stddef.h>

/* ===== Retained md.h ===== */

/**
 * \file md.h
 *
 * \brief   This file contains the generic functions for message-digest
 *          (hashing) and HMAC.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/md.h.
 */

#ifndef MBEDTLS_MD_H
#define MBEDTLS_MD_H

#include <stddef.h>


/** The selected feature is not available. */
#define MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE                -0x5080
/** Bad input parameters to function. */
#define MBEDTLS_ERR_MD_BAD_INPUT_DATA                     -0x5100
/** Failed to allocate memory. */
#define MBEDTLS_ERR_MD_ALLOC_FAILED                       -0x5180
/** Opening or reading of file failed. */
#define MBEDTLS_ERR_MD_FILE_IO_ERROR                      -0x5200

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief     Supported message digests.
 *
 * \warning   MD5 and SHA-1 are considered weak message digests and
 *            their use constitutes a security risk. We recommend considering
 *            stronger message digests instead.
 *
 */
/* Note: these are aligned with the definitions of PSA_ALG_ macros for hashes,
 * in order to enable an efficient implementation of conversion functions.
 * This is tested by md_to_from_psa() in test_suite_md. */
typedef enum {
    MBEDTLS_MD_NONE=0,    /**< None. */
    MBEDTLS_MD_MD5=0x03,       /**< The MD5 message digest. */
    MBEDTLS_MD_RIPEMD160=0x04, /**< The RIPEMD-160 message digest. */
    MBEDTLS_MD_SHA1=0x05,      /**< The SHA-1 message digest. */
    MBEDTLS_MD_SHA224=0x08,    /**< The SHA-224 message digest. */
    MBEDTLS_MD_SHA256=0x09,    /**< The SHA-256 message digest. */
    MBEDTLS_MD_SHA384=0x0a,    /**< The SHA-384 message digest. */
    MBEDTLS_MD_SHA512=0x0b,    /**< The SHA-512 message digest. */
    MBEDTLS_MD_SHA3_224=0x10,  /**< The SHA3-224 message digest. */
    MBEDTLS_MD_SHA3_256=0x11,  /**< The SHA3-256 message digest. */
    MBEDTLS_MD_SHA3_384=0x12,  /**< The SHA3-384 message digest. */
    MBEDTLS_MD_SHA3_512=0x13,  /**< The SHA3-512 message digest. */
} mbedtls_md_type_t;

/* Note: this should always be >= PSA_HASH_MAX_SIZE
 * in all builds with both CRYPTO_C and MD_LIGHT.
 *
 * This is to make things easier for modules such as TLS that may define a
 * buffer size using MD_MAX_SIZE in a part of the code that's common to PSA
 * and legacy, then assume the buffer's size is PSA_HASH_MAX_SIZE in another
 * part of the code based on PSA.
 */
#define MBEDTLS_MD_MAX_SIZE         64  /* longest known is SHA512 */

#define MBEDTLS_MD_MAX_BLOCK_SIZE         128

/**
 * Opaque struct.
 *
 * Constructed using either #mbedtls_md_info_from_string or
 * #mbedtls_md_info_from_type.
 *
 * Fields can be accessed with #mbedtls_md_get_size,
 * #mbedtls_md_get_type and #mbedtls_md_get_name.
 */
/* Defined internally in library/md_wrap.h. */
typedef struct mbedtls_md_info_t mbedtls_md_info_t;

/**
 * Used internally to indicate whether a context uses legacy or PSA.
 *
 * Internal use only.
 */
typedef enum {
    MBEDTLS_MD_ENGINE_LEGACY = 0,
    MBEDTLS_MD_ENGINE_PSA,
} mbedtls_md_engine_t;

/**
 * The generic message-digest context.
 */
typedef struct mbedtls_md_context_t {
    /** Information about the associated message digest. */
    const mbedtls_md_info_t *MBEDTLS_PRIVATE(md_info);

    /** The digest-specific context (legacy) or the PSA operation. */
    void *MBEDTLS_PRIVATE(md_ctx);

    /** The HMAC part of the context. */
    void *MBEDTLS_PRIVATE(hmac_ctx);
} mbedtls_md_context_t;

/**
 * \brief           This function returns the message-digest information
 *                  associated with the given digest type.
 *
 * \param md_type   The type of digest to search for.
 *
 * \return          The message-digest information associated with \p md_type.
 * \return          NULL if the associated message-digest information is not found.
 */
const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t md_type);

/**
 * \brief           This function initializes a message-digest context without
 *                  binding it to a particular message-digest algorithm.
 *
 *                  This function should always be called first. It prepares the
 *                  context for mbedtls_md_setup() for binding it to a
 *                  message-digest algorithm.
 */
void mbedtls_md_init(mbedtls_md_context_t *ctx);

/**
 * \brief           This function clears the internal structure of \p ctx and
 *                  frees any embedded internal structure, but does not free
 *                  \p ctx itself.
 *
 *                  If you have called mbedtls_md_setup() on \p ctx, you must
 *                  call mbedtls_md_free() when you are no longer using the
 *                  context.
 *                  Calling this function if you have previously
 *                  called mbedtls_md_init() and nothing else is optional.
 *                  You must not call this function if you have not called
 *                  mbedtls_md_init().
 */
void mbedtls_md_free(mbedtls_md_context_t *ctx);

/**
 * \brief           This function selects the message digest algorithm to use,
 *                  and allocates internal structures.
 *
 *                  It should be called after mbedtls_md_init() or
 *                  mbedtls_md_free(). Makes it necessary to call
 *                  mbedtls_md_free() later.
 *
 * \param ctx       The context to set up.
 * \param md_info   The information structure of the message-digest algorithm
 *                  to use.
 * \param hmac      Defines if HMAC is used. 0: HMAC is not used (saves some memory),
 *                  or non-zero: HMAC is used with this context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 * \return          #MBEDTLS_ERR_MD_ALLOC_FAILED on memory-allocation failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_setup(mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function clones the state of a message-digest
 *                  context.
 *
 * \note            You must call mbedtls_md_setup() on \c dst before calling
 *                  this function.
 *
 * \note            The two contexts must have the same type,
 *                  for example, both are SHA-256.
 *
 * \warning         This function clones the message-digest state, not the
 *                  HMAC state.
 *
 * \param dst       The destination context.
 * \param src       The context to be cloned.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification failure.
 * \return          #MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE if both contexts are
 *                  not using the same engine. This can be avoided by moving
 *                  the call to psa_crypto_init() before the first call to
 *                  mbedtls_md_setup().
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_clone(mbedtls_md_context_t *dst,
                     const mbedtls_md_context_t *src, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function extracts the message-digest size from the
 *                  message-digest information structure.
 *
 * \param md_info   The information structure of the message-digest algorithm
 *                  to use.
 *
 * \return          The size of the message-digest output in Bytes.
 */
unsigned char mbedtls_md_get_size(const mbedtls_md_info_t *md_info);

/**
 * \brief           This function gives the message-digest size associated to
 *                  message-digest type.
 *
 * \param md_type   The message-digest type.
 *
 * \return          The size of the message-digest output in Bytes,
 *                  or 0 if the message-digest type is not known.
 */
static inline unsigned char mbedtls_md_get_size_from_type(mbedtls_md_type_t md_type)
{
    return mbedtls_md_get_size(mbedtls_md_info_from_type(md_type));
}

/**
 * \brief           This function starts a message-digest computation.
 *
 *                  You must call this function after setting up the context
 *                  with mbedtls_md_setup(), and before passing data with
 *                  mbedtls_md_update().
 *
 * \param ctx       The generic message-digest context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_starts(mbedtls_md_context_t *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function feeds an input buffer into an ongoing
 *                  message-digest computation.
 *
 *                  You must call mbedtls_md_starts() before calling this
 *                  function. You may call this function multiple times.
 *                  Afterwards, call mbedtls_md_finish().
 *
 * \param ctx       The generic message-digest context.
 * \param input     The buffer holding the input data.
 * \param ilen      The length of the input data.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function finishes the digest operation,
 *                  and writes the result to the output buffer.
 *
 *                  Call this function after a call to mbedtls_md_starts(),
 *                  followed by any number of calls to mbedtls_md_update().
 *                  Afterwards, you may either clear the context with
 *                  mbedtls_md_free(), or call mbedtls_md_starts() to reuse
 *                  the context for another digest operation with the same
 *                  algorithm.
 *
 * \param ctx       The generic message-digest context.
 * \param output    The buffer for the generic message-digest checksum result.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_finish(mbedtls_md_context_t *ctx, unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function calculates the message-digest of a buffer,
 *                 with respect to a configurable message-digest algorithm
 *                 in a single call.
 *
 *                 The result is calculated as
 *                 Output = message_digest(input buffer).
 *
 * \param md_info  The information structure of the message-digest algorithm
 *                 to use.
 * \param input    The buffer holding the data.
 * \param ilen     The length of the input data.
 * \param output   The generic message-digest checksum result.
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                 failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md(const mbedtls_md_info_t *md_info, const unsigned char *input, size_t ilen,
               unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function returns the message-digest information
 *                  from the given context.
 *
 * \param ctx       The context from which to extract the information.
 *                  This must be initialized (or \c NULL).
 *
 * \return          The message-digest information associated with \p ctx.
 * \return          \c NULL if \p ctx is \c NULL.
 */
const mbedtls_md_info_t *mbedtls_md_info_from_ctx(
    const mbedtls_md_context_t *ctx);

/**
 * \brief           This function sets the HMAC key and prepares to
 *                  authenticate a new message.
 *
 *                  Call this function after mbedtls_md_setup(), to use
 *                  the MD context for an HMAC calculation, then call
 *                  mbedtls_md_hmac_update() to provide the input data, and
 *                  mbedtls_md_hmac_finish() to get the HMAC value.
 *
 * \param ctx       The message digest context containing an embedded HMAC
 *                  context.
 * \param key       The HMAC secret key.
 * \param keylen    The length of the HMAC key in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_hmac_starts(mbedtls_md_context_t *ctx, const unsigned char *key,
                           size_t keylen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function feeds an input buffer into an ongoing HMAC
 *                  computation.
 *
 *                  Call mbedtls_md_hmac_starts() or mbedtls_md_hmac_reset()
 *                  before calling this function.
 *                  You may call this function multiple times to pass the
 *                  input piecewise.
 *                  Afterwards, call mbedtls_md_hmac_finish().
 *
 * \param ctx       The message digest context containing an embedded HMAC
 *                  context.
 * \param input     The buffer holding the input data.
 * \param ilen      The length of the input data.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_hmac_update(mbedtls_md_context_t *ctx, const unsigned char *input,
                           size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function finishes the HMAC operation, and writes
 *                  the result to the output buffer.
 *
 *                  Call this function after mbedtls_md_hmac_starts() and
 *                  mbedtls_md_hmac_update() to get the HMAC value. Afterwards
 *                  you may either call mbedtls_md_free() to clear the context,
 *                  or call mbedtls_md_hmac_reset() to reuse the context with
 *                  the same HMAC key.
 *
 * \param ctx       The message digest context containing an embedded HMAC
 *                  context.
 * \param output    The generic HMAC checksum result.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_hmac_finish(mbedtls_md_context_t *ctx, unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function prepares to authenticate a new message with
 *                  the same key as the previous HMAC operation.
 *
 *                  You may call this function after mbedtls_md_hmac_finish().
 *                  Afterwards call mbedtls_md_hmac_update() to pass the new
 *                  input.
 *
 * \param ctx       The message digest context containing an embedded HMAC
 *                  context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_MD_BAD_INPUT_DATA on parameter-verification
 *                  failure.
 */
MBEDTLS_CHECK_RETURN_TYPICAL
int mbedtls_md_hmac_reset(mbedtls_md_context_t *ctx, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_MD_H */


#if (defined(MBEDTLS_MD_CAN_SHA512) || defined(PSA_WANT_ALG_SHA_512)) && \
    !defined(MBEDTLS_ENTROPY_FORCE_SHA256)
#define MBEDTLS_ENTROPY_SHA512_ACCUMULATOR
#define MBEDTLS_ENTROPY_MD  MBEDTLS_MD_SHA512
#define MBEDTLS_ENTROPY_BLOCK_SIZE      64      /**< Block size of entropy accumulator (SHA-512) */
#else
#define MBEDTLS_ENTROPY_SHA256_ACCUMULATOR
#define MBEDTLS_ENTROPY_MD  MBEDTLS_MD_SHA256
#define MBEDTLS_ENTROPY_BLOCK_SIZE      32      /**< Block size of entropy accumulator (SHA-256) */
#endif

/** Critical entropy source failure. */
#define MBEDTLS_ERR_ENTROPY_SOURCE_FAILED                 -0x003C
/** No more sources can be added. */
#define MBEDTLS_ERR_ENTROPY_MAX_SOURCES                   -0x003E
/** No sources have been added to poll. */
#define MBEDTLS_ERR_ENTROPY_NO_SOURCES_DEFINED            -0x0040
/** No strong sources have been added to poll. */
#define MBEDTLS_ERR_ENTROPY_NO_STRONG_SOURCE              -0x003D
/** Read/write error in file. */
#define MBEDTLS_ERR_ENTROPY_FILE_IO_ERROR                 -0x003F

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h or define them on the compiler command line.
 * \{
 */

#if !defined(MBEDTLS_ENTROPY_MAX_SOURCES)
#define MBEDTLS_ENTROPY_MAX_SOURCES     20      /**< Maximum number of sources supported */
#endif

#if !defined(MBEDTLS_ENTROPY_MAX_GATHER)
#define MBEDTLS_ENTROPY_MAX_GATHER      128     /**< Maximum amount requested from entropy sources */
#endif

/** \} name SECTION: Module settings */

#define MBEDTLS_ENTROPY_MAX_SEED_SIZE   1024    /**< Maximum size of seed we read from seed file */
#define MBEDTLS_ENTROPY_SOURCE_MANUAL   MBEDTLS_ENTROPY_MAX_SOURCES

#define MBEDTLS_ENTROPY_SOURCE_STRONG   1       /**< Entropy source is strong   */
#define MBEDTLS_ENTROPY_SOURCE_WEAK     0       /**< Entropy source is weak     */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           Entropy poll callback pointer
 *
 * \param data      Callback-specific data pointer
 * \param output    Data to fill
 * \param len       Maximum size to provide
 * \param olen      The actual amount of bytes put into the buffer (Can be 0)
 *
 * \return          0 if no critical failures occurred,
 *                  MBEDTLS_ERR_ENTROPY_SOURCE_FAILED otherwise
 */
typedef int (*mbedtls_entropy_f_source_ptr)(void *data, unsigned char *output, size_t len,
                                            size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Entropy source state
 */
typedef struct mbedtls_entropy_source_state {
    mbedtls_entropy_f_source_ptr    MBEDTLS_PRIVATE(f_source);   /**< The entropy source callback */
    void *MBEDTLS_PRIVATE(p_source);             /**< The callback data pointer */
    size_t          MBEDTLS_PRIVATE(size);       /**< Amount received in bytes */
    size_t          MBEDTLS_PRIVATE(threshold);  /**< Minimum bytes required before release */
    int             MBEDTLS_PRIVATE(strong);     /**< Is the source strong? */
}
mbedtls_entropy_source_state;

/**
 * \brief           Entropy context structure
 */
typedef struct mbedtls_entropy_context {
    mbedtls_md_context_t  MBEDTLS_PRIVATE(accumulator);
    int MBEDTLS_PRIVATE(accumulator_started); /* 0 after init.
                                               * 1 after the first update.
                                               * -1 after free. */
    int             MBEDTLS_PRIVATE(source_count); /* Number of entries used in source. */
    mbedtls_entropy_source_state    MBEDTLS_PRIVATE(source)[MBEDTLS_ENTROPY_MAX_SOURCES];
}
mbedtls_entropy_context;

/**
 * \brief           Platform-specific entropy poll callback
 */
int mbedtls_platform_entropy_poll(void *data,
                                  unsigned char *output, size_t len, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Initialize the context
 *
 * \param ctx       Entropy context to initialize
 */
void mbedtls_entropy_init(mbedtls_entropy_context *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Free the data in the context
 *
 * \param ctx       Entropy context to free
 */
void mbedtls_entropy_free(mbedtls_entropy_context *ctx);

/**
 * \brief           Adds an entropy source to poll
 *                  (Thread-safe if MBEDTLS_THREADING_C is enabled)
 *
 * \param ctx       Entropy context
 * \param f_source  Entropy function
 * \param p_source  Function data
 * \param threshold Minimum required from source before entropy is released
 *                  ( with mbedtls_entropy_func() ) (in bytes)
 * \param strong    MBEDTLS_ENTROPY_SOURCE_STRONG or
 *                  MBEDTLS_ENTROPY_SOURCE_WEAK.
 *                  At least one strong source needs to be added.
 *                  Weaker sources (such as the cycle counter) can be used as
 *                  a complement.
 *
 * \return          0 if successful or MBEDTLS_ERR_ENTROPY_MAX_SOURCES
 */
int mbedtls_entropy_add_source(mbedtls_entropy_context *ctx,
                               mbedtls_entropy_f_source_ptr f_source, void *p_source,
                               size_t threshold, int strong, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Retrieve entropy from the accumulator
 *                  (Maximum length: MBEDTLS_ENTROPY_BLOCK_SIZE)
 *                  (Thread-safe if MBEDTLS_THREADING_C is enabled)
 *
 * \param data      Entropy context
 * \param output    Buffer to fill
 * \param len       Number of bytes desired, must be at most MBEDTLS_ENTROPY_BLOCK_SIZE
 *
 * \return          0 if successful, or MBEDTLS_ERR_ENTROPY_SOURCE_FAILED
 */
int mbedtls_entropy_func(void *data, unsigned char *output, size_t len, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* entropy.h */


/** The entropy source failed. */
#define MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED        -0x0034
/** The requested random buffer length is too big. */
#define MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG              -0x0036
/** The input (entropy + additional data) is too large. */
#define MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG                -0x0038
/** Read or write error in file. */
#define MBEDTLS_ERR_CTR_DRBG_FILE_IO_ERROR                -0x003A

#define MBEDTLS_CTR_DRBG_BLOCKSIZE          16 /**< The block size used by the cipher. */

#if defined(MBEDTLS_CTR_DRBG_USE_128_BIT_KEY)
#define MBEDTLS_CTR_DRBG_KEYSIZE            16
/**< The key size in bytes used by the cipher.
 *
 * Compile-time choice: 16 bytes (128 bits)
 * because #MBEDTLS_CTR_DRBG_USE_128_BIT_KEY is enabled.
 */
#else
#define MBEDTLS_CTR_DRBG_KEYSIZE            32
/**< The key size in bytes used by the cipher.
 *
 * Compile-time choice: 32 bytes (256 bits)
 * because \c MBEDTLS_CTR_DRBG_USE_128_BIT_KEY is disabled.
 */
#endif

#define MBEDTLS_CTR_DRBG_KEYBITS            (MBEDTLS_CTR_DRBG_KEYSIZE * 8)   /**< The key size for the DRBG operation, in bits. */
#define MBEDTLS_CTR_DRBG_SEEDLEN            (MBEDTLS_CTR_DRBG_KEYSIZE + MBEDTLS_CTR_DRBG_BLOCKSIZE)   /**< The seed length, calculated as (counter + AES key). */

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h or define them using the compiler command
 * line.
 * \{
 */

/** \def MBEDTLS_CTR_DRBG_ENTROPY_LEN
 *
 * \brief The amount of entropy used per seed by default, in bytes.
 */
#if !defined(MBEDTLS_CTR_DRBG_ENTROPY_LEN)
#if defined(MBEDTLS_ENTROPY_SHA512_ACCUMULATOR)
/** This is 48 bytes because the entropy module uses SHA-512.
 */
#define MBEDTLS_CTR_DRBG_ENTROPY_LEN        48

#else

/** This is 32 bytes because the entropy module uses SHA-256.
 */
#if !defined(MBEDTLS_CTR_DRBG_USE_128_BIT_KEY)
/** \warning To achieve a 256-bit security strength, you must pass a nonce
 *           to mbedtls_ctr_drbg_seed().
 */
#endif /* !defined(MBEDTLS_CTR_DRBG_USE_128_BIT_KEY) */
#define MBEDTLS_CTR_DRBG_ENTROPY_LEN        32
#endif /* MBEDTLS_ENTROPY_SHA512_ACCUMULATOR */
#endif /* !defined(MBEDTLS_CTR_DRBG_ENTROPY_LEN) */

#if !defined(MBEDTLS_CTR_DRBG_RESEED_INTERVAL)
#define MBEDTLS_CTR_DRBG_RESEED_INTERVAL    10000
/**< The interval before reseed is performed by default. */
#endif

#if !defined(MBEDTLS_CTR_DRBG_MAX_INPUT)
#define MBEDTLS_CTR_DRBG_MAX_INPUT          256
/**< The maximum number of additional input Bytes. */
#endif

#if !defined(MBEDTLS_CTR_DRBG_MAX_REQUEST)
#define MBEDTLS_CTR_DRBG_MAX_REQUEST        1024
/**< The maximum number of requested Bytes per call. */
#endif

#if !defined(MBEDTLS_CTR_DRBG_MAX_SEED_INPUT)
#define MBEDTLS_CTR_DRBG_MAX_SEED_INPUT     384
/**< The maximum size of seed or reseed buffer in bytes. */
#endif

/** \} name SECTION: Module settings */

#define MBEDTLS_CTR_DRBG_PR_OFF             0
/**< Prediction resistance is disabled. */
#define MBEDTLS_CTR_DRBG_PR_ON              1
/**< Prediction resistance is enabled. */

#ifdef __cplusplus
extern "C" {
#endif

#if MBEDTLS_CTR_DRBG_ENTROPY_LEN >= MBEDTLS_CTR_DRBG_KEYSIZE * 3 / 2
/** The default length of the nonce read from the entropy source.
 *
 * This is \c 0 because a single read from the entropy source is sufficient
 * to include a nonce.
 * See the documentation of mbedtls_ctr_drbg_seed() for more information.
 */
#define MBEDTLS_CTR_DRBG_ENTROPY_NONCE_LEN 0
#else
/** The default length of the nonce read from the entropy source.
 *
 * This is half of the default entropy length because a single read from
 * the entropy source does not provide enough material to form a nonce.
 * See the documentation of mbedtls_ctr_drbg_seed() for more information.
 */
#define MBEDTLS_CTR_DRBG_ENTROPY_NONCE_LEN (MBEDTLS_CTR_DRBG_ENTROPY_LEN + 1) / 2
#endif

/**
 * \brief          The CTR_DRBG context structure.
 */
typedef struct mbedtls_ctr_drbg_context {
    unsigned char MBEDTLS_PRIVATE(counter)[16];  /*!< The counter (V). */
    int MBEDTLS_PRIVATE(reseed_counter);         /*!< The reseed counter.
                                                  * This is the number of requests that have
                                                  * been made since the last (re)seeding.
                                                  * Before the initial seeding, this field
                                                  * contains the amount of entropy in bytes
                                                  * to use as a nonce for the initial seeding,
                                                  * or -1 if no nonce length has been explicitly
                                                  * set (see mbedtls_ctr_drbg_set_nonce_len()).
                                                  */
    int MBEDTLS_PRIVATE(prediction_resistance);  /*!< This determines whether prediction
                                                    resistance is enabled, that is
                                                    whether to systematically reseed before
                                                    each random generation. */
    size_t MBEDTLS_PRIVATE(entropy_len);         /*!< The amount of entropy grabbed on each
                                                    seed or reseed operation, in bytes. */
    int MBEDTLS_PRIVATE(reseed_interval);        /*!< The reseed interval.
                                                  * This is the maximum number of requests
                                                  * that can be made between reseedings. */

    mbedtls_aes_context MBEDTLS_PRIVATE(aes_ctx);        /*!< The AES context. */

    /*
     * Callbacks (Entropy)
     */
    int(*MBEDTLS_PRIVATE(f_entropy))(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    /*!< The entropy callback function. */

    void *MBEDTLS_PRIVATE(p_entropy);            /*!< The context for the entropy function. */

}
mbedtls_ctr_drbg_context;

/**
 * \brief               This function initializes the CTR_DRBG context,
 *                      and prepares it for mbedtls_ctr_drbg_seed()
 *                      or mbedtls_ctr_drbg_free().
 *
 * \note                The reseed interval is
 *                      #MBEDTLS_CTR_DRBG_RESEED_INTERVAL by default.
 *                      You can override it by calling
 *                      mbedtls_ctr_drbg_set_reseed_interval().
 *
 * \param ctx           The CTR_DRBG context to initialize.
 */
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx);

/**
 * \brief               This function seeds and sets up the CTR_DRBG
 *                      entropy source for future reseeds.
 *
 * A typical choice for the \p f_entropy and \p p_entropy parameters is
 * to use the entropy module:
 * - \p f_entropy is mbedtls_entropy_func();
 * - \p p_entropy is an instance of ::mbedtls_entropy_context initialized
 *   with mbedtls_entropy_init() (which registers the platform's default
 *   entropy sources).
 *
 * The entropy length is #MBEDTLS_CTR_DRBG_ENTROPY_LEN by default.
 * You can override it by calling mbedtls_ctr_drbg_set_entropy_len().
 *
 * The entropy nonce length is:
 * - \c 0 if the entropy length is at least 3/2 times the entropy length,
 *   which guarantees that the security strength is the maximum permitted
 *   by the key size and entropy length according to NIST SP 800-90A §10.2.1;
 * - Half the entropy length otherwise.
 * You can override it by calling mbedtls_ctr_drbg_set_nonce_len().
 * With the default entropy length, the entropy nonce length is
 * #MBEDTLS_CTR_DRBG_ENTROPY_NONCE_LEN.
 *
 * You can provide a nonce and personalization string in addition to the
 * entropy source, to make this instantiation as unique as possible.
 * See SP 800-90A §8.6.7 for more details about nonces.
 *
 * The _seed_material_ value passed to the derivation function in
 * the CTR_DRBG Instantiate Process described in NIST SP 800-90A §10.2.1.3.2
 * is the concatenation of the following strings:
 * - A string obtained by calling \p f_entropy function for the entropy
 *   length.
 */
#if MBEDTLS_CTR_DRBG_ENTROPY_NONCE_LEN == 0
/**
 * - If mbedtls_ctr_drbg_set_nonce_len() has been called, a string
 *   obtained by calling \p f_entropy function for the specified length.
 */
#else
/**
 * - A string obtained by calling \p f_entropy function for the entropy nonce
 *   length. If the entropy nonce length is \c 0, this function does not
 *   make a second call to \p f_entropy.
 */
#endif
/**
 * - The \p custom string.
 *
 * \note                To achieve the nominal security strength permitted
 *                      by CTR_DRBG, the entropy length must be:
 *                      - at least 16 bytes for a 128-bit strength
 *                      (maximum achievable strength when using AES-128);
 *                      - at least 32 bytes for a 256-bit strength
 *                      (maximum achievable strength when using AES-256).
 *
 *                      In addition, if you do not pass a nonce in \p custom,
 *                      the sum of the entropy length
 *                      and the entropy nonce length must be:
 *                      - at least 24 bytes for a 128-bit strength
 *                      (maximum achievable strength when using AES-128);
 *                      - at least 48 bytes for a 256-bit strength
 *                      (maximum achievable strength when using AES-256).
 *
 * \param ctx           The CTR_DRBG context to seed.
 *                      It must have been initialized with
 *                      mbedtls_ctr_drbg_init().
 *                      After a successful call to mbedtls_ctr_drbg_seed(),
 *                      you may not call mbedtls_ctr_drbg_seed() again on
 *                      the same context unless you call
 *                      mbedtls_ctr_drbg_free() and mbedtls_ctr_drbg_init()
 *                      again first.
 *                      After a failed call to mbedtls_ctr_drbg_seed(),
 *                      you must call mbedtls_ctr_drbg_free().
 * \param f_entropy     The entropy callback, taking as arguments the
 *                      \p p_entropy context, the buffer to fill, and the
 *                      length of the buffer.
 *                      \p f_entropy is always called with a buffer size
 *                      less than or equal to the entropy length.
 * \param p_entropy     The entropy context to pass to \p f_entropy.
 * \param custom        The personalization string.
 *                      This can be \c NULL, in which case the personalization
 *                      string is empty regardless of the value of \p len.
 * \param len           The length of the personalization string.
 *                      This must be at most
 *                      #MBEDTLS_CTR_DRBG_MAX_SEED_INPUT
 *                      - #MBEDTLS_CTR_DRBG_ENTROPY_LEN.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED on failure.
 */
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                          int (*f_entropy)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                          void *p_entropy,
                          const unsigned char *custom,
                          size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief               This function resets CTR_DRBG context to the state immediately
 *                      after initial call of mbedtls_ctr_drbg_init().
 *
 * \param ctx           The CTR_DRBG context to clear.
 */
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx);

/**
 * \brief               This function reseeds the CTR_DRBG context, that is
 *                      extracts data from the entropy source.
 *
 * \note                This function is not thread-safe. It is not safe
 *                      to call this function if another thread might be
 *                      concurrently obtaining random numbers from the same
 *                      context or updating or reseeding the same context.
 *
 * \param ctx           The CTR_DRBG context.
 * \param additional    Additional data to add to the state. Can be \c NULL.
 * \param len           The length of the additional data.
 *                      This must be less than
 *                      #MBEDTLS_CTR_DRBG_MAX_SEED_INPUT - \c entropy_len
 *                      where \c entropy_len is the entropy length
 *                      configured for the context.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED on failure.
 */
int mbedtls_ctr_drbg_reseed(mbedtls_ctr_drbg_context *ctx,
                            const unsigned char *additional, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief   This function updates a CTR_DRBG instance with additional
 *          data and uses it to generate random data.
 *
 * This function automatically reseeds if the reseed counter is exceeded
 * or prediction resistance is enabled.
 *
 * \note                This function is not thread-safe. It is not safe
 *                      to call this function if another thread might be
 *                      concurrently obtaining random numbers from the same
 *                      context or updating or reseeding the same context.
 *
 * \param p_rng         The CTR_DRBG context. This must be a pointer to a
 *                      #mbedtls_ctr_drbg_context structure.
 * \param output        The buffer to fill.
 * \param output_len    The length of the buffer in bytes.
 * \param additional    Additional data to update. Can be \c NULL, in which
 *                      case the additional data is empty regardless of
 *                      the value of \p add_len.
 * \param add_len       The length of the additional data
 *                      if \p additional is not \c NULL.
 *                      This must be less than #MBEDTLS_CTR_DRBG_MAX_INPUT
 *                      and less than
 *                      #MBEDTLS_CTR_DRBG_MAX_SEED_INPUT - \c entropy_len
 *                      where \c entropy_len is the entropy length
 *                      configured for the context.
 *
 * \return    \c 0 on success.
 * \return    #MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED or
 *            #MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG on failure.
 */
int mbedtls_ctr_drbg_random_with_add(void *p_rng,
                                     unsigned char *output, size_t output_len,
                                     const unsigned char *additional, size_t add_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief   This function uses CTR_DRBG to generate random data.
 *
 * This function automatically reseeds if the reseed counter is exceeded
 * or prediction resistance is enabled.
 */
/**
 * \param p_rng         The CTR_DRBG context. This must be a pointer to a
 *                      #mbedtls_ctr_drbg_context structure.
 * \param output        The buffer to fill.
 * \param output_len    The length of the buffer in bytes.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED or
 *                      #MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG on failure.
 */
int mbedtls_ctr_drbg_random(void *p_rng,
                            unsigned char *output, size_t output_len, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* ctr_drbg.h */

/* ===== Retained debug.h ===== */

/**
 * \file debug.h
 *
 * \brief Functions for controlling and providing debug output from the library.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/debug.h.
 */
#ifndef MBEDTLS_DEBUG_H
#define MBEDTLS_DEBUG_H


/* ===== Retained ssl.h ===== */

/**
 * \file ssl.h
 *
 * \brief SSL/TLS functions.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ssl.h.
 */
#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H



/* ===== Retained ssl_ciphersuites.h ===== */

/**
 * \file ssl_ciphersuites.h
 *
 * \brief SSL Ciphersuites for Mbed TLS
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ssl_ciphersuites.h.
 */
#ifndef MBEDTLS_SSL_CIPHERSUITES_H
#define MBEDTLS_SSL_CIPHERSUITES_H


/* ===== Retained pk.h ===== */

/**
 * \file pk.h
 *
 * \brief Public Key abstraction layer
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/pk.h.
 */

#ifndef MBEDTLS_PK_H
#define MBEDTLS_PK_H



/* ===== Retained rsa.h ===== */

/**
 * \file rsa.h
 *
 * \brief This file provides an API for the RSA public-key cryptosystem.
 *
 * The RSA public-key cryptosystem is defined in <em>Public-Key
 * Cryptography Standards (PKCS) #1 v1.5: RSA Encryption</em>
 * and <em>Public-Key Cryptography Standards (PKCS) #1 v2.1:
 * RSA Cryptography Specifications</em>.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/rsa.h.
 */
#ifndef MBEDTLS_RSA_H
#define MBEDTLS_RSA_H



/*
 * RSA Error codes
 */
/** Bad input parameters to function. */
#define MBEDTLS_ERR_RSA_BAD_INPUT_DATA                    -0x4080
/** Input data contains invalid padding and is rejected. */
#define MBEDTLS_ERR_RSA_INVALID_PADDING                   -0x4100
/** Something failed during generation of a key. */
#define MBEDTLS_ERR_RSA_KEY_GEN_FAILED                    -0x4180
/** Key failed to pass the validity check of the library. */
#define MBEDTLS_ERR_RSA_KEY_CHECK_FAILED                  -0x4200
/** The public key operation failed. */
#define MBEDTLS_ERR_RSA_PUBLIC_FAILED                     -0x4280
/** The private key operation failed. */
#define MBEDTLS_ERR_RSA_PRIVATE_FAILED                    -0x4300
/** The PKCS#1 verification failed. */
#define MBEDTLS_ERR_RSA_VERIFY_FAILED                     -0x4380
/** The output buffer for decryption is not large enough. */
#define MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE                  -0x4400
/** The random generator failed to generate non-zeros. */
#define MBEDTLS_ERR_RSA_RNG_FAILED                        -0x4480

/*
 * RSA constants
 */

#define MBEDTLS_RSA_PKCS_V15    0 /**< Use PKCS#1 v1.5 encoding. */
#define MBEDTLS_RSA_PKCS_V21    1 /**< Use PKCS#1 v2.1 encoding. */

#define MBEDTLS_RSA_SIGN        1 /**< Identifier for RSA signature operations. */
#define MBEDTLS_RSA_CRYPT       2 /**< Identifier for RSA encryption and decryption operations. */

#define MBEDTLS_RSA_SALT_LEN_ANY    -1

/*
 * The above constants may be used even if the RSA module is compile out,
 * eg for alternative (PKCS#11) RSA implementations in the PK layers.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Regular implementation
//

#if !defined(MBEDTLS_RSA_GEN_KEY_MIN_BITS)
#define MBEDTLS_RSA_GEN_KEY_MIN_BITS 1024
#elif MBEDTLS_RSA_GEN_KEY_MIN_BITS < 128
#error "MBEDTLS_RSA_GEN_KEY_MIN_BITS must be at least 128 bits"
#endif

/**
 * \brief   The RSA context structure.
 */
typedef struct mbedtls_rsa_context {
    int MBEDTLS_PRIVATE(ver);                    /*!<  Reserved for internal purposes.
                                                  *    Do not set this field in application
                                                  *    code. Its meaning might change without
                                                  *    notice. */
    size_t MBEDTLS_PRIVATE(len);                 /*!<  The size of \p N in Bytes. */

    mbedtls_mpi MBEDTLS_PRIVATE(N);              /*!<  The public modulus. */
    mbedtls_mpi MBEDTLS_PRIVATE(E);              /*!<  The public exponent. */

    mbedtls_mpi MBEDTLS_PRIVATE(D);              /*!<  The private exponent. */
    mbedtls_mpi MBEDTLS_PRIVATE(P);              /*!<  The first prime factor. */
    mbedtls_mpi MBEDTLS_PRIVATE(Q);              /*!<  The second prime factor. */

    mbedtls_mpi MBEDTLS_PRIVATE(DP);             /*!<  <code>D % (P - 1)</code>. */
    mbedtls_mpi MBEDTLS_PRIVATE(DQ);             /*!<  <code>D % (Q - 1)</code>. */
    mbedtls_mpi MBEDTLS_PRIVATE(QP);             /*!<  <code>1 / (Q % P)</code>. */

    mbedtls_mpi MBEDTLS_PRIVATE(RN);             /*!<  cached <code>R^2 mod N</code>. */

    mbedtls_mpi MBEDTLS_PRIVATE(RP);             /*!<  cached <code>R^2 mod P</code>. */
    mbedtls_mpi MBEDTLS_PRIVATE(RQ);             /*!<  cached <code>R^2 mod Q</code>. */

    mbedtls_mpi MBEDTLS_PRIVATE(Vi);             /*!<  The cached blinding value. */
    mbedtls_mpi MBEDTLS_PRIVATE(Vf);             /*!<  The cached un-blinding value. */

    int MBEDTLS_PRIVATE(padding);                /*!< Selects padding mode:
                                                  #MBEDTLS_RSA_PKCS_V15 for 1.5 padding and
                                                  #MBEDTLS_RSA_PKCS_V21 for OAEP or PSS. */
    int MBEDTLS_PRIVATE(hash_id);                /*!< Hash identifier of mbedtls_md_type_t type,
                                                    as specified in md.h for use in the MGF
                                                    mask generating function used in the
                                                    EME-OAEP and EMSA-PSS encodings. */
}
mbedtls_rsa_context;

/**
 * \brief          This function initializes an RSA context.
 *
 * \note           This function initializes the padding and the hash
 *                 identifier to respectively #MBEDTLS_RSA_PKCS_V15 and
 *                 #MBEDTLS_MD_NONE. See mbedtls_rsa_set_padding() for more
 *                 information about those parameters.
 *
 * \param ctx      The RSA context to initialize. This must not be \c NULL.
 */
void mbedtls_rsa_init(mbedtls_rsa_context *ctx);

/**
 * \brief          This function imports core RSA parameters, in raw big-endian
 *                 binary format, into an RSA context.
 *
 * \note           This function can be called multiple times for successive
 *                 imports, if the parameters are not simultaneously present.
 *
 *                 Any sequence of calls to this function should be followed
 *                 by a call to mbedtls_rsa_complete(), which checks and
 *                 completes the provided information to a ready-for-use
 *                 public or private RSA key.
 *
 * \note           See mbedtls_rsa_complete() for more information on which
 *                 parameters are necessary to set up a private or public
 *                 RSA key.
 *
 * \note           The imported parameters are copied and need not be preserved
 *                 for the lifetime of the RSA context being set up.
 *
 * \param ctx      The initialized RSA context to store the parameters in.
 * \param N        The RSA modulus. This may be \c NULL.
 * \param N_len    The Byte length of \p N; it is ignored if \p N == NULL.
 * \param P        The first prime factor of \p N. This may be \c NULL.
 * \param P_len    The Byte length of \p P; it is ignored if \p P == NULL.
 * \param Q        The second prime factor of \p N. This may be \c NULL.
 * \param Q_len    The Byte length of \p Q; it is ignored if \p Q == NULL.
 * \param D        The private exponent. This may be \c NULL.
 * \param D_len    The Byte length of \p D; it is ignored if \p D == NULL.
 * \param E        The public exponent. This may be \c NULL.
 * \param E_len    The Byte length of \p E; it is ignored if \p E == NULL.
 *
 * \return         \c 0 on success.
 * \return         A non-zero error code on failure.
 */
int mbedtls_rsa_import_raw(mbedtls_rsa_context *ctx,
                           unsigned char const *N, size_t N_len,
                           unsigned char const *P, size_t P_len,
                           unsigned char const *Q, size_t Q_len,
                           unsigned char const *D, size_t D_len,
                           unsigned char const *E, size_t E_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function completes an RSA context from
 *                 a set of imported core parameters.
 *
 *                 To setup an RSA public key, precisely \c N and \c E
 *                 must have been imported.
 *
 *                 To setup an RSA private key, sufficient information must
 *                 be present for the other parameters to be derivable.
 *
 *                 The default implementation supports the following:
 *                 <ul><li>Derive \c P, \c Q from \c N, \c D, \c E.</li>
 *                 <li>Derive \c N, \c D from \c P, \c Q, \c E.</li></ul>
 *                 Alternative implementations need not support these.
 *
 *                 If this function runs successfully, it guarantees that
 *                 the RSA context can be used for RSA operations without
 *                 the risk of failure or crash.
 *
 * \warning        This function need not perform consistency checks
 *                 for the imported parameters. In particular, parameters that
 *                 are not needed by the implementation might be silently
 *                 discarded and left unchecked. To check the consistency
 *                 of the key material, see mbedtls_rsa_check_privkey().
 *
 * \param ctx      The initialized RSA context holding imported parameters.
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_RSA_BAD_INPUT_DATA if the attempted derivations
 *                 failed.
 *
 */
int mbedtls_rsa_complete(mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function retrieves the length of the RSA modulus in bits.
 *
 * \param ctx      The initialized RSA context.
 *
 * \return         The length of the RSA modulus in bits.
 *
 */
size_t mbedtls_rsa_get_bitlen(const mbedtls_rsa_context *ctx);

/**
 * \brief          This function retrieves the length of RSA modulus in Bytes.
 *
 * \param ctx      The initialized RSA context.
 *
 * \return         The length of the RSA modulus in Bytes.
 *
 */
size_t mbedtls_rsa_get_len(const mbedtls_rsa_context *ctx);

/**
 * \brief          This function checks if a context contains at least an RSA
 *                 public key.
 *
 *                 If the function runs successfully, it is guaranteed that
 *                 enough information is present to perform an RSA public key
 *                 operation using mbedtls_rsa_public().
 *
 * \param ctx      The initialized RSA context to check.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 *
 */
int mbedtls_rsa_check_pubkey(const mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief      This function checks if a context contains an RSA private key
 *             and perform basic consistency checks.
 *
 * \note       The consistency checks performed by this function not only
 *             ensure that mbedtls_rsa_private() can be called successfully
 *             on the given context, but that the various parameters are
 *             mutually consistent with high probability, in the sense that
 *             mbedtls_rsa_public() and mbedtls_rsa_private() are inverses.
 *
 * \warning    This function should catch accidental misconfigurations
 *             like swapping of parameters, but it cannot establish full
 *             trust in neither the quality nor the consistency of the key
 *             material that was used to setup the given RSA context:
 *             <ul><li>Consistency: Imported parameters that are irrelevant
 *             for the implementation might be silently dropped. If dropped,
 *             the current function does not have access to them,
 *             and therefore cannot check them. See mbedtls_rsa_complete().
 *             If you want to check the consistency of the entire
 *             content of a PKCS1-encoded RSA private key, for example, you
 *             should use mbedtls_rsa_validate_params() before setting
 *             up the RSA context.
 *             Additionally, if the implementation performs empirical checks,
 *             these checks substantiate but do not guarantee consistency.</li>
 *             <li>Quality: This function is not expected to perform
 *             extended quality assessments like checking that the prime
 *             factors are safe. Additionally, it is the responsibility of the
 *             user to ensure the trustworthiness of the source of his RSA
 *             parameters, which goes beyond what is effectively checkable
 *             by the library.</li></ul>
 *
 * \param ctx  The initialized RSA context to check.
 *
 * \return     \c 0 on success.
 * \return     An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_check_privkey(const mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function checks a public-private RSA key pair.
 *
 *                 It checks each of the contexts, and makes sure they match.
 *
 * \param pub      The initialized RSA context holding the public key.
 * \param prv      The initialized RSA context holding the private key.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_check_pub_priv(const mbedtls_rsa_context *pub,
                               const mbedtls_rsa_context *prv, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs an RSA public key operation.
 *
 * \param ctx      The initialized RSA context to use.
 * \param input    The input buffer. This must be a readable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 * \param output   The output buffer. This must be a writable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \note           This function does not handle message padding.
 *
 * \note           Make sure to set \p input[0] = 0 or ensure that
 *                 input is smaller than \c N.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_public(mbedtls_rsa_context *ctx,
                       const unsigned char *input,
                       unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs an RSA private key operation.
 *
 * \note           Blinding is used if and only if a PRNG is provided.
 *
 * \note           If blinding is used, both the base of exponentiation
 *                 and the exponent are blinded, providing protection
 *                 against some side-channel attacks.
 *
 * \warning        It is deprecated and a security risk to not provide
 *                 a PRNG here and thereby prevent the use of blinding.
 *                 Future versions of the library may enforce the presence
 *                 of a PRNG.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function, used for blinding. It is mandatory.
 * \param p_rng    The RNG context to pass to \p f_rng. This may be \c NULL
 *                 if \p f_rng doesn't need a context.
 * \param input    The input buffer. This must be a readable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 * \param output   The output buffer. This must be a writable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 *
 */
int mbedtls_rsa_private(mbedtls_rsa_context *ctx,
                        mbedtls_f_rng_t *f_rng,
                        void *p_rng,
                        const unsigned char *input,
                        unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function adds the message padding, then performs an RSA
 *                 operation.
 *
 *                 It is the generic wrapper for performing a PKCS#1 encryption
 *                 operation.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG to use. It is used for padding generation
 *                 and it is mandatory.
 * \param p_rng    The RNG context to be passed to \p f_rng. May be
 *                 \c NULL if \p f_rng doesn't need a context argument.
 * \param ilen     The length of the plaintext in Bytes.
 * \param input    The input data to encrypt. This must be a readable
 *                 buffer of size \p ilen Bytes. It may be \c NULL if
 *                 `ilen == 0`.
 * \param output   The output buffer. This must be a writable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_pkcs1_encrypt(mbedtls_rsa_context *ctx,
                              mbedtls_f_rng_t *f_rng,
                              void *p_rng,
                              size_t ilen,
                              const unsigned char *input,
                              unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v1.5 encryption operation
 *                 (RSAES-PKCS1-v1_5-ENCRYPT).
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function to use. It is mandatory and used for
 *                 padding generation.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may
 *                 be \c NULL if \p f_rng doesn't need a context argument.
 * \param ilen     The length of the plaintext in Bytes.
 * \param input    The input data to encrypt. This must be a readable
 *                 buffer of size \p ilen Bytes. It may be \c NULL if
 *                 `ilen == 0`.
 * \param output   The output buffer. This must be a writable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsaes_pkcs1_v15_encrypt(mbedtls_rsa_context *ctx,
                                        mbedtls_f_rng_t *f_rng,
                                        void *p_rng,
                                        size_t ilen,
                                        const unsigned char *input,
                                        unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief            This function performs a PKCS#1 v2.1 OAEP encryption
 *                   operation (RSAES-OAEP-ENCRYPT).
 *
 * \note             The output buffer must be as large as the size
 *                   of ctx->N. For example, 128 Bytes if RSA-1024 is used.
 *
 * \param ctx        The initialized RSA context to use.
 * \param f_rng      The RNG function to use. This is needed for padding
 *                   generation and is mandatory.
 * \param p_rng      The RNG context to be passed to \p f_rng. This may
 *                   be \c NULL if \p f_rng doesn't need a context argument.
 * \param label      The buffer holding the custom label to use.
 *                   This must be a readable buffer of length \p label_len
 *                   Bytes. It may be \c NULL if \p label_len is \c 0.
 * \param label_len  The length of the label in Bytes.
 * \param ilen       The length of the plaintext buffer \p input in Bytes.
 * \param input      The input data to encrypt. This must be a readable
 *                   buffer of size \p ilen Bytes. It may be \c NULL if
 *                   `ilen == 0`.
 * \param output     The output buffer. This must be a writable buffer
 *                   of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                   for an 2048-bit RSA modulus.
 *
 * \return           \c 0 on success.
 * \return           An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsaes_oaep_encrypt(mbedtls_rsa_context *ctx,
                                   mbedtls_f_rng_t *f_rng,
                                   void *p_rng,
                                   const unsigned char *label, size_t label_len,
                                   size_t ilen,
                                   const unsigned char *input,
                                   unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs an RSA operation, then removes the
 *                 message padding.
 *
 *                 It is the generic wrapper for performing a PKCS#1 decryption
 *                 operation.
 *
 * \warning        When \p ctx->padding is set to #MBEDTLS_RSA_PKCS_V15,
 *                 mbedtls_rsa_rsaes_pkcs1_v15_decrypt() is called, which is an
 *                 inherently dangerous function (CWE-242). In that case, the
 *                 return value of this function is sensitive, see the
 *                 documentation of mbedtls_rsa_rsaes_pkcs1_v15_decrypt().
 *
 * \note           The output buffer length \c output_max_len should be
 *                 as large as the size \p ctx->len of \p ctx->N (for example,
 *                 128 Bytes if RSA-1024 is used) to be able to hold an
 *                 arbitrary decrypted message. If it is not large enough to
 *                 hold the decryption of the particular ciphertext provided,
 *                 the function returns \c MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function. This is used for blinding and is
 *                 mandatory; see mbedtls_rsa_private() for more.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may be
 *                 \c NULL if \p f_rng doesn't need a context.
 * \param olen     The address at which to store the length of
 *                 the plaintext. This must not be \c NULL.
 * \param input    The ciphertext buffer. This must be a readable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 * \param output   The buffer used to hold the plaintext. This must
 *                 be a writable buffer of length \p output_max_len Bytes.
 * \param output_max_len The length in Bytes of the output buffer \p output.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_pkcs1_decrypt(mbedtls_rsa_context *ctx,
                              mbedtls_f_rng_t *f_rng,
                              void *p_rng,
                              size_t *olen,
                              const unsigned char *input,
                              unsigned char *output,
                              size_t output_max_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v1.5 decryption
 *                 operation (RSAES-PKCS1-v1_5-DECRYPT).
 *
 * \warning        This is an inherently dangerous function (CWE-242). Unless
 *                 it is used in a side channel free and safe way,
 *                 the calling code is vulnerable.
 *                 Specifically, callers need to ensure an adversary cannot
 *                 distinguish between success, MBEDTLS_ERR_RSA_INVALID_PADDING
 *                 and MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE. Also, in the latter two
 *                 cases, the values of the output bytes must be ignored, again
 *                 without revealing whether that's the case.
 *
 * \note           The output buffer length \c output_max_len should be
 *                 as large as the size \p ctx->len of \p ctx->N, for example,
 *                 128 Bytes if RSA-1024 is used, to be able to hold an
 *                 arbitrary decrypted message. If it is not large enough to
 *                 hold the decryption of the particular ciphertext provided,
 *                 the function returns #MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function. This is used for blinding and is
 *                 mandatory; see mbedtls_rsa_private() for more.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may be
 *                 \c NULL if \p f_rng doesn't need a context.
 * \param olen     The address at which to store the length of
 *                 the plaintext. This must not be \c NULL.
 * \param input    The ciphertext buffer. This must be a readable buffer
 *                 of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 * \param output   The buffer used to hold the plaintext. This must
 *                 be a writable buffer of length \p output_max_len Bytes.
 * \param output_max_len The length in Bytes of the output buffer \p output.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 *
 */
int mbedtls_rsa_rsaes_pkcs1_v15_decrypt(mbedtls_rsa_context *ctx,
                                        mbedtls_f_rng_t *f_rng,
                                        void *p_rng,
                                        size_t *olen,
                                        const unsigned char *input,
                                        unsigned char *output,
                                        size_t output_max_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief            This function performs a PKCS#1 v2.1 OAEP decryption
 *                   operation (RSAES-OAEP-DECRYPT).
 *
 * \note             The output buffer length \c output_max_len should be
 *                   as large as the size \p ctx->len of \p ctx->N, for
 *                   example, 128 Bytes if RSA-1024 is used, to be able to
 *                   hold an arbitrary decrypted message. If it is not
 *                   large enough to hold the decryption of the particular
 *                   ciphertext provided, the function returns
 *                   #MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE.
 *
 * \param ctx        The initialized RSA context to use.
 * \param f_rng      The RNG function. This is used for blinding and is
 *                   mandatory.
 * \param p_rng      The RNG context to be passed to \p f_rng. This may be
 *                   \c NULL if \p f_rng doesn't need a context.
 * \param label      The buffer holding the custom label to use.
 *                   This must be a readable buffer of length \p label_len
 *                   Bytes. It may be \c NULL if \p label_len is \c 0.
 * \param label_len  The length of the label in Bytes.
 * \param olen       The address at which to store the length of
 *                   the plaintext. This must not be \c NULL.
 * \param input      The ciphertext buffer. This must be a readable buffer
 *                   of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                   for an 2048-bit RSA modulus.
 * \param output     The buffer used to hold the plaintext. This must
 *                   be a writable buffer of length \p output_max_len Bytes.
 * \param output_max_len The length in Bytes of the output buffer \p output.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsaes_oaep_decrypt(mbedtls_rsa_context *ctx,
                                   mbedtls_f_rng_t *f_rng,
                                   void *p_rng,
                                   const unsigned char *label, size_t label_len,
                                   size_t *olen,
                                   const unsigned char *input,
                                   unsigned char *output,
                                   size_t output_max_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a private RSA operation to sign
 *                 a message digest using PKCS#1.
 *
 *                 It is the generic wrapper for performing a PKCS#1
 *                 signature.
 *
 * \note           The \p sig buffer must be as large as the size
 *                 of \p ctx->N. For example, 128 Bytes if RSA-1024 is used.
 *
 * \note           For PKCS#1 v2.1 encoding, see comments on
 *                 mbedtls_rsa_rsassa_pss_sign() for details on
 *                 \p md_alg and \p hash_id.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function to use. This is mandatory and
 *                 must not be \c NULL.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may be \c NULL
 *                 if \p f_rng doesn't need a context argument.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer to hold the signature. This must be a writable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus. A buffer length of
 *                 #MBEDTLS_MPI_MAX_SIZE is always safe.
 *
 * \return         \c 0 if the signing operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_pkcs1_sign(mbedtls_rsa_context *ctx,
                           mbedtls_f_rng_t *f_rng,
                           void *p_rng,
                           mbedtls_md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v1.5 signature
 *                 operation (RSASSA-PKCS1-v1_5-SIGN).
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function. This is used for blinding and is
 *                 mandatory; see mbedtls_rsa_private() for more.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may be \c NULL
 *                 if \p f_rng doesn't need a context argument.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer to hold the signature. This must be a writable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus. A buffer length of
 *                 #MBEDTLS_MPI_MAX_SIZE is always safe.
 *
 * \return         \c 0 if the signing operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsassa_pkcs1_v15_sign(mbedtls_rsa_context *ctx,
                                      mbedtls_f_rng_t *f_rng,
                                      void *p_rng,
                                      mbedtls_md_type_t md_alg,
                                      unsigned int hashlen,
                                      const unsigned char *hash,
                                      unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v2.1 PSS signature
 *                 operation (RSASSA-PSS-SIGN).
 *
 * \note           The \c hash_id set in \p ctx by calling
 *                 mbedtls_rsa_set_padding() selects the hash used for the
 *                 encoding operation and for the mask generation function
 *                 (MGF1). For more details on the encoding operation and the
 *                 mask generation function, consult <em>RFC-3447: Public-Key
 *                 Cryptography Standards (PKCS) #1 v2.1: RSA Cryptography
 *                 Specifications</em>.
 *
 * \note           This function always uses the maximum possible salt size,
 *                 up to the length of the payload hash. This choice of salt
 *                 size complies with FIPS 186-4 §5.5 (e) and RFC 8017 (PKCS#1
 *                 v2.2) §9.1.1 step 3. Furthermore this function enforces a
 *                 minimum salt size which is the hash size minus 2 bytes. If
 *                 this minimum size is too large given the key size (the salt
 *                 size, plus the hash size, plus 2 bytes must be no more than
 *                 the key size in bytes), this function returns
 *                 #MBEDTLS_ERR_RSA_BAD_INPUT_DATA.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function. It is mandatory and must not be \c NULL.
 * \param p_rng    The RNG context to be passed to \p f_rng. This may be \c NULL
 *                 if \p f_rng doesn't need a context argument.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer to hold the signature. This must be a writable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus. A buffer length of
 *                 #MBEDTLS_MPI_MAX_SIZE is always safe.
 *
 * \return         \c 0 if the signing operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsassa_pss_sign(mbedtls_rsa_context *ctx,
                                mbedtls_f_rng_t *f_rng,
                                void *p_rng,
                                mbedtls_md_type_t md_alg,
                                unsigned int hashlen,
                                const unsigned char *hash,
                                unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a public RSA operation and checks
 *                 the message digest.
 *
 *                 This is the generic wrapper for performing a PKCS#1
 *                 verification.
 *
 * \note           For PKCS#1 v2.1 encoding, see comments on
 *                 mbedtls_rsa_rsassa_pss_verify() about \c md_alg and
 *                 \c hash_id.
 *
 * \param ctx      The initialized RSA public key context to use.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer holding the signature. This must be a readable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 if the verify operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_pkcs1_verify(mbedtls_rsa_context *ctx,
                             mbedtls_md_type_t md_alg,
                             unsigned int hashlen,
                             const unsigned char *hash,
                             const unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v1.5 verification
 *                 operation (RSASSA-PKCS1-v1_5-VERIFY).
 *
 * \param ctx      The initialized RSA public key context to use.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer holding the signature. This must be a readable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 if the verify operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsassa_pkcs1_v15_verify(mbedtls_rsa_context *ctx,
                                        mbedtls_md_type_t md_alg,
                                        unsigned int hashlen,
                                        const unsigned char *hash,
                                        const unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v2.1 PSS verification
 *                 operation (RSASSA-PSS-VERIFY).
 *
 * \note           The \c hash_id set in \p ctx by calling
 *                 mbedtls_rsa_set_padding() selects the hash used for the
 *                 encoding operation and for the mask generation function
 *                 (MGF1). For more details on the encoding operation and the
 *                 mask generation function, consult <em>RFC-3447: Public-Key
 *                 Cryptography Standards (PKCS) #1 v2.1: RSA Cryptography
 *                 Specifications</em>. If the \c hash_id set in \p ctx by
 *                 mbedtls_rsa_set_padding() is #MBEDTLS_MD_NONE, the \p md_alg
 *                 parameter is used.
 *
 * \param ctx      The initialized RSA public key context to use.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param sig      The buffer holding the signature. This must be a readable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 if the verify operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsassa_pss_verify(mbedtls_rsa_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  unsigned int hashlen,
                                  const unsigned char *hash,
                                  const unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function performs a PKCS#1 v2.1 PSS verification
 *                 operation (RSASSA-PSS-VERIFY).
 *
 * \note           The \p sig buffer must be as large as the size
 *                 of \p ctx->N. For example, 128 Bytes if RSA-1024 is used.
 *
 * \note           The \c hash_id set in \p ctx by mbedtls_rsa_set_padding() is
 *                 ignored.
 *
 * \param ctx      The initialized RSA public key context to use.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Use #MBEDTLS_MD_NONE for signing raw data.
 * \param hashlen  The length of the message digest or raw data in Bytes.
 *                 If \p md_alg is not #MBEDTLS_MD_NONE, this must match the
 *                 output length of the corresponding hash algorithm.
 * \param hash     The buffer holding the message digest or raw data.
 *                 This must be a readable buffer of at least \p hashlen Bytes.
 * \param mgf1_hash_id      The message digest algorithm used for the
 *                          verification operation and the mask generation
 *                          function (MGF1). For more details on the encoding
 *                          operation and the mask generation function, consult
 *                          <em>RFC-3447: Public-Key Cryptography Standards
 *                          (PKCS) #1 v2.1: RSA Cryptography
 *                          Specifications</em>.
 * \param expected_salt_len The length of the salt used in padding. Use
 *                          #MBEDTLS_RSA_SALT_LEN_ANY to accept any salt length.
 * \param sig      The buffer holding the signature. This must be a readable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 if the verify operation was successful.
 * \return         An \c MBEDTLS_ERR_RSA_XXX error code on failure.
 */
int mbedtls_rsa_rsassa_pss_verify_ext(mbedtls_rsa_context *ctx,
                                      mbedtls_md_type_t md_alg,
                                      unsigned int hashlen,
                                      const unsigned char *hash,
                                      mbedtls_md_type_t mgf1_hash_id,
                                      int expected_salt_len,
                                      const unsigned char *sig, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function frees the components of an RSA key.
 *
 * \param ctx      The RSA context to free. May be \c NULL, in which case
 *                 this function is a no-op. If it is not \c NULL, it must
 *                 point to an initialized RSA context.
 */
void mbedtls_rsa_free(mbedtls_rsa_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* rsa.h */



/* ===== Retained ecdsa.h ===== */

/**
 * \file ecdsa.h
 *
 * \brief This file contains ECDSA definitions and functions.
 *
 * The Elliptic Curve Digital Signature Algorithm (ECDSA) is defined in
 * <em>Standards for Efficient Cryptography Group (SECG):
 * SEC1 Elliptic Curve Cryptography</em>.
 * The use of ECDSA for TLS is defined in <em>RFC-4492: Elliptic Curve
 * Cryptography (ECC) Cipher Suites for Transport Layer Security (TLS)</em>.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ecdsa.h.
 */

#ifndef MBEDTLS_ECDSA_H
#define MBEDTLS_ECDSA_H



/**
 * \brief           Maximum ECDSA signature size for a given curve bit size
 *
 * \param bits      Curve size in bits
 * \return          Maximum signature size in bytes
 *
 * \note            This macro returns a compile-time constant if its argument
 *                  is one. It may evaluate its argument multiple times.
 */
/*
 *     Ecdsa-Sig-Value ::= SEQUENCE {
 *         r       INTEGER,
 *         s       INTEGER
 *     }
 *
 * For each of r and s, the value (V) may include an extra initial "0" bit.
 */
#define MBEDTLS_ECDSA_MAX_SIG_LEN(bits)                               \
    (/*T,L of SEQUENCE*/ ((bits) >= 61 * 8 ? 3 : 2) +              \
     /*T,L of r,s*/ 2 * (((bits) >= 127 * 8 ? 3 : 2) +     \
                         /*V of r,s*/ ((bits) + 8) / 8))

/** The maximal size of an ECDSA signature in Bytes. */
#define MBEDTLS_ECDSA_MAX_LEN  MBEDTLS_ECDSA_MAX_SIG_LEN(MBEDTLS_ECP_MAX_BITS)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           The ECDSA context structure.
 *
 * \warning         Performing multiple operations concurrently on the same
 *                  ECDSA context is not supported; objects of this type
 *                  should not be shared between multiple threads.
 *
 * \note            pk_wrap module assumes that "ecdsa_context" is identical
 *                  to "ecp_keypair" (see for example structure
 *                  "mbedtls_eckey_info" where ECDSA sign/verify functions
 *                  are used also for EC key)
 */
typedef mbedtls_ecp_keypair mbedtls_ecdsa_context;

/* Now we can declare functions that take a pointer to that */
typedef void mbedtls_ecdsa_restart_ctx;

/**
 * \brief          This function checks whether a given group can be used
 *                 for ECDSA.
 *
 * \param gid      The ECP group ID to check.
 *
 * \return         \c 1 if the group can be used, \c 0 otherwise
 */
int mbedtls_ecdsa_can_do(mbedtls_ecp_group_id gid);

/**
 * \brief           This function computes the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            The deterministic version implemented in
 *                  mbedtls_ecdsa_sign_det_ext() is usually preferred.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated
 *                  as defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.3, step 5.
 *
 * \see             ecp.h
 *
 * \param grp       The context for the elliptic curve to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param r         The MPI context in which to store the first part
 *                  the signature. This must be initialized.
 * \param s         The MPI context in which to store the second part
 *                  the signature. This must be initialized.
 * \param d         The private signing key. This must be initialized.
 * \param buf       The content to be signed. This is usually the hash of
 *                  the original data to be signed. This must be a readable
 *                  buffer of length \p blen Bytes. It may be \c NULL if
 *                  \p blen is zero.
 * \param blen      The length of \p buf in Bytes.
 * \param f_rng     The RNG function, used both to generate the ECDSA nonce
 *                  and for blinding. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context parameter.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX
 *                  or \c MBEDTLS_MPI_XXX error code on failure.
 */
int mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                       const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                       mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_ECDSA_SIGN_ALT)
/**
 * \brief               This function computes the ECDSA signature of a
 *                      previously-hashed message, in a restartable way.
 *
 * \note                The deterministic version implemented in
 *                      mbedtls_ecdsa_sign_det_restartable() is usually
 *                      preferred.
 *
 * \note                This function is like \c mbedtls_ecdsa_sign() but
 *                      it can return early and restart according to the
 *                      limit set with \c mbedtls_ecp_set_max_ops() to
 *                      reduce blocking.
 *
 * \note                If the bitlength of the message hash is larger
 *                      than the bitlength of the group order, then the
 *                      hash is truncated as defined in <em>Standards for
 *                      Efficient Cryptography Group (SECG): SEC1 Elliptic
 *                      Curve Cryptography</em>, section 4.1.3, step 5.
 *
 * \see                 ecp.h
 *
 * \param grp           The context for the elliptic curve to use.
 *                      This must be initialized and have group parameters
 *                      set, for example through mbedtls_ecp_group_load().
 * \param r             The MPI context in which to store the first part
 *                      the signature. This must be initialized.
 * \param s             The MPI context in which to store the second part
 *                      the signature. This must be initialized.
 * \param d             The private signing key. This must be initialized
 *                      and setup, for example through
 *                      mbedtls_ecp_gen_privkey().
 * \param buf           The hashed content to be signed. This must be a readable
 *                      buffer of length \p blen Bytes. It may be \c NULL if
 *                      \p blen is zero.
 * \param blen          The length of \p buf in Bytes.
 * \param f_rng         The RNG function used to generate the ECDSA nonce.
 *                      This must not be \c NULL.
 * \param p_rng         The RNG context to be passed to \p f_rng. This may be
 *                      \c NULL if \p f_rng doesn't need a context parameter.
 * \param f_rng_blind   The RNG function used for blinding. This must not be
 *                      \c NULL.
 * \param p_rng_blind   The RNG context to be passed to \p f_rng. This may be
 *                      \c NULL if \p f_rng doesn't need a context parameter.
 * \param rs_ctx        The restart context to use. This may be \c NULL
 *                      to disable restarting. If it is not \c NULL, it
 *                      must point to an initialized restart context.
 *
 * \return              \c 0 on success.
 * \return              #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                      operations was reached: see \c
 *                      mbedtls_ecp_set_max_ops().
 * \return              Another \c MBEDTLS_ERR_ECP_XXX, \c
 *                      MBEDTLS_ERR_MPI_XXX or \c MBEDTLS_ERR_ASN1_XXX
 *                      error code on failure.
 */
int mbedtls_ecdsa_sign_restartable(
    mbedtls_ecp_group *grp,
    mbedtls_mpi *r, mbedtls_mpi *s,
    const mbedtls_mpi *d,
    const unsigned char *buf, size_t blen,
    mbedtls_f_rng_t *f_rng,
    void *p_rng,
    mbedtls_f_rng_t *f_rng_blind,
    void *p_rng_blind,
    mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

#endif /* !MBEDTLS_ECDSA_SIGN_ALT */

/**
 * \brief           This function verifies the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.4, step 3.
 *
 * \see             ecp.h
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param buf       The hashed content that was signed. This must be a readable
 *                  buffer of length \p blen Bytes. It may be \c NULL if
 *                  \p blen is zero.
 * \param blen      The length of \p buf in Bytes.
 * \param Q         The public key to use for verification. This must be
 *                  initialized and setup.
 * \param r         The first integer of the signature.
 *                  This must be initialized.
 * \param s         The second integer of the signature.
 *                  This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX or \c MBEDTLS_MPI_XXX
 *                  error code on failure.
 */
int mbedtls_ecdsa_verify(mbedtls_ecp_group *grp,
                         const unsigned char *buf, size_t blen,
                         const mbedtls_ecp_point *Q, const mbedtls_mpi *r,
                         const mbedtls_mpi *s, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_ECDSA_VERIFY_ALT)
/**
 * \brief           This function verifies the ECDSA signature of a
 *                  previously-hashed message, in a restartable manner
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.4, step 3.
 *
 * \see             ecp.h
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param buf       The hashed content that was signed. This must be a readable
 *                  buffer of length \p blen Bytes. It may be \c NULL if
 *                  \p blen is zero.
 * \param blen      The length of \p buf in Bytes.
 * \param Q         The public key to use for verification. This must be
 *                  initialized and setup.
 * \param r         The first integer of the signature.
 *                  This must be initialized.
 * \param s         The second integer of the signature.
 *                  This must be initialized.
 * \param rs_ctx    The restart context to use. This may be \c NULL to disable
 *                  restarting. If it is not \c NULL, it must point to an
 *                  initialized restart context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          An \c MBEDTLS_ERR_ECP_XXX or \c MBEDTLS_MPI_XXX
 *                  error code on failure.
 */
int mbedtls_ecdsa_verify_restartable(mbedtls_ecp_group *grp,
                                     const unsigned char *buf, size_t blen,
                                     const mbedtls_ecp_point *Q,
                                     const mbedtls_mpi *r,
                                     const mbedtls_mpi *s,
                                     mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

#endif /* !MBEDTLS_ECDSA_VERIFY_ALT */

/**
 * \brief           This function computes the ECDSA signature and writes it
 *                  to a buffer, serialized as defined in <em>RFC-4492:
 *                  Elliptic Curve Cryptography (ECC) Cipher Suites for
 *                  Transport Layer Security (TLS)</em>.
 *
 * \warning         It is not thread-safe to use the same context in
 *                  multiple threads.
 *
 * \note            The deterministic version is used if
 *                  #MBEDTLS_ECDSA_DETERMINISTIC is defined. For more
 *                  information, see <em>RFC-6979: Deterministic Usage
 *                  of the Digital Signature Algorithm (DSA) and Elliptic
 *                  Curve Digital Signature Algorithm (ECDSA)</em>.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.3, step 5.
 *
 * \see             ecp.h
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and private key bound to it, for example
 *                  via mbedtls_ecdsa_genkey() or mbedtls_ecdsa_from_keypair().
 * \param md_alg    The message digest that was used to hash the message.
 * \param hash      The message hash to be signed. This must be a readable
 *                  buffer of length \p hlen Bytes.
 * \param hlen      The length of the hash \p hash in Bytes.
 * \param sig       The buffer to which to write the signature. This must be a
 *                  writable buffer of length at least twice as large as the
 *                  size of the curve used, plus 9. For example, 73 Bytes if
 *                  a 256-bit curve is used. A buffer length of
 *                  #MBEDTLS_ECDSA_MAX_LEN is always safe.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param slen      The address at which to store the actual length of
 *                  the signature written. Must not be \c NULL.
 * \param f_rng     The RNG function. This is used for blinding.
 *                  If #MBEDTLS_ECDSA_DETERMINISTIC is unset, this is also
 *                  used to generate the ECDSA nonce.
 *                  This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't use a context.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX, \c MBEDTLS_ERR_MPI_XXX or
 *                  \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_ecdsa_write_signature(mbedtls_ecdsa_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hlen,
                                  unsigned char *sig, size_t sig_size, size_t *slen,
                                  mbedtls_f_rng_t *f_rng,
                                  void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function computes the ECDSA signature and writes it
 *                  to a buffer, in a restartable way.
 *
 * \see             \c mbedtls_ecdsa_write_signature()
 *
 * \note            This function is like \c mbedtls_ecdsa_write_signature()
 *                  but it can return early and restart according to the limit
 *                  set with \c mbedtls_ecp_set_max_ops() to reduce blocking.
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and private key bound to it, for example
 *                  via mbedtls_ecdsa_genkey() or mbedtls_ecdsa_from_keypair().
 * \param md_alg    The message digest that was used to hash the message.
 * \param hash      The message hash to be signed. This must be a readable
 *                  buffer of length \p hlen Bytes.
 * \param hlen      The length of the hash \p hash in Bytes.
 * \param sig       The buffer to which to write the signature. This must be a
 *                  writable buffer of length at least twice as large as the
 *                  size of the curve used, plus 9. For example, 73 Bytes if
 *                  a 256-bit curve is used. A buffer length of
 *                  #MBEDTLS_ECDSA_MAX_LEN is always safe.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param slen      The address at which to store the actual length of
 *                  the signature written. Must not be \c NULL.
 * \param f_rng     The RNG function. This is used for blinding.
 *                  If #MBEDTLS_ECDSA_DETERMINISTIC is unset, this is also
 *                  used to generate the ECDSA nonce.
 *                  This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't use a context.
 * \param rs_ctx    The restart context to use. This may be \c NULL to disable
 *                  restarting. If it is not \c NULL, it must point to an
 *                  initialized restart context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX, \c MBEDTLS_ERR_MPI_XXX or
 *                  \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_ecdsa_write_signature_restartable(mbedtls_ecdsa_context *ctx,
                                              mbedtls_md_type_t md_alg,
                                              const unsigned char *hash, size_t hlen,
                                              unsigned char *sig, size_t sig_size, size_t *slen,
                                              mbedtls_f_rng_t *f_rng,
                                              void *p_rng,
                                              mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function reads and verifies an ECDSA signature.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.4, step 3.
 *
 * \see             ecp.h
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and public key bound to it.
 * \param hash      The message hash that was signed. This must be a readable
 *                  buffer of length \p hlen Bytes.
 * \param hlen      The size of the hash \p hash.
 * \param sig       The signature to read and verify. This must be a readable
 *                  buffer of length \p slen Bytes.
 * \param slen      The size of \p sig in Bytes.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if signature is invalid.
 * \return          #MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig, but its length is less than \p siglen.
 * \return          An \c MBEDTLS_ERR_ECP_XXX or \c MBEDTLS_ERR_MPI_XXX
 *                  error code on failure for any other reason.
 */
int mbedtls_ecdsa_read_signature(mbedtls_ecdsa_context *ctx,
                                 const unsigned char *hash, size_t hlen,
                                 const unsigned char *sig, size_t slen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function reads and verifies an ECDSA signature,
 *                  in a restartable way.
 *
 * \see             \c mbedtls_ecdsa_read_signature()
 *
 * \note            This function is like \c mbedtls_ecdsa_read_signature()
 *                  but it can return early and restart according to the limit
 *                  set with \c mbedtls_ecp_set_max_ops() to reduce blocking.
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and public key bound to it.
 * \param hash      The message hash that was signed. This must be a readable
 *                  buffer of length \p hlen Bytes.
 * \param hlen      The size of the hash \p hash.
 * \param sig       The signature to read and verify. This must be a readable
 *                  buffer of length \p slen Bytes.
 * \param slen      The size of \p sig in Bytes.
 * \param rs_ctx    The restart context to use. This may be \c NULL to disable
 *                  restarting. If it is not \c NULL, it must point to an
 *                  initialized restart context.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_BAD_INPUT_DATA if signature is invalid.
 * \return          #MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig, but its length is less than \p siglen.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX or \c MBEDTLS_ERR_MPI_XXX
 *                  error code on failure for any other reason.
 */
int mbedtls_ecdsa_read_signature_restartable(mbedtls_ecdsa_context *ctx,
                                             const unsigned char *hash, size_t hlen,
                                             const unsigned char *sig, size_t slen,
                                             mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* ecdsa.h */


/** Memory allocation failed. */
#define MBEDTLS_ERR_PK_ALLOC_FAILED        -0x3F80
/** Type mismatch, eg attempt to encrypt with an ECDSA key */
#define MBEDTLS_ERR_PK_TYPE_MISMATCH       -0x3F00
/** Bad input parameters to function. */
#define MBEDTLS_ERR_PK_BAD_INPUT_DATA      -0x3E80
/** Read/write of file failed. */
#define MBEDTLS_ERR_PK_FILE_IO_ERROR       -0x3E00
/** Unsupported key version */
#define MBEDTLS_ERR_PK_KEY_INVALID_VERSION -0x3D80
/** Invalid key tag or value. */
#define MBEDTLS_ERR_PK_KEY_INVALID_FORMAT  -0x3D00
/** Key algorithm is unsupported (only RSA and EC are supported). */
#define MBEDTLS_ERR_PK_UNKNOWN_PK_ALG      -0x3C80
/** Private key password can't be empty. */
#define MBEDTLS_ERR_PK_PASSWORD_REQUIRED   -0x3C00
/** Given private key password does not allow for correct decryption. */
#define MBEDTLS_ERR_PK_PASSWORD_MISMATCH   -0x3B80
/** The pubkey tag or value is invalid (only RSA and EC are supported). */
#define MBEDTLS_ERR_PK_INVALID_PUBKEY      -0x3B00
/** The algorithm tag or value is invalid. */
#define MBEDTLS_ERR_PK_INVALID_ALG         -0x3A80
/** Elliptic curve is unsupported (only NIST curves are supported). */
#define MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE -0x3A00
/** Unavailable feature, e.g. RSA disabled for RSA key. */
#define MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE -0x3980
/** The buffer contains a valid signature followed by more data. */
#define MBEDTLS_ERR_PK_SIG_LEN_MISMATCH    -0x3900
/** The output buffer is too small. */
#define MBEDTLS_ERR_PK_BUFFER_TOO_SMALL    -0x3880

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Public key types
 */
typedef enum {
    MBEDTLS_PK_NONE=0,
    MBEDTLS_PK_RSA,
    MBEDTLS_PK_ECKEY,
    MBEDTLS_PK_ECKEY_DH,
    MBEDTLS_PK_ECDSA,
    MBEDTLS_PK_RSA_ALT,
    MBEDTLS_PK_RSASSA_PSS,
    MBEDTLS_PK_OPAQUE,
} mbedtls_pk_type_t;

/**
 * \brief           Options for RSASSA-PSS signature verification.
 *                  See \c mbedtls_rsa_rsassa_pss_verify_ext()
 */
typedef struct mbedtls_pk_rsassa_pss_options {
    /** The digest to use for MGF1 in PSS.
     *
     * \note When #MBEDTLS_USE_PSA_CRYPTO is enabled and #MBEDTLS_RSA_C is
     *       disabled, this must be equal to the \c md_alg argument passed
     *       to mbedtls_pk_verify_ext(). In a future version of the library,
     *       this constraint may apply whenever #MBEDTLS_USE_PSA_CRYPTO is
     *       enabled regardless of the status of #MBEDTLS_RSA_C.
     */
    mbedtls_md_type_t mgf1_hash_id;

    /** The expected length of the salt, in bytes. This may be
     * #MBEDTLS_RSA_SALT_LEN_ANY to accept any salt length.
     *
     * \note When #MBEDTLS_USE_PSA_CRYPTO is enabled, only
     *       #MBEDTLS_RSA_SALT_LEN_ANY is valid. Any other value may be
     *       ignored (allowing any salt length).
     */
    int expected_salt_len;

} mbedtls_pk_rsassa_pss_options;

/**
 * \brief           Maximum size of a signature made by mbedtls_pk_sign().
 */
/* We need to set MBEDTLS_PK_SIGNATURE_MAX_SIZE to the maximum signature
 * size among the supported signature types. Do it by starting at 0,
 * then incrementally increasing to be large enough for each supported
 * signature mechanism.
 *
 * The resulting value can be 0, for example if MBEDTLS_ECDH_C is enabled
 * (which allows the pk module to be included) but neither MBEDTLS_ECDSA_C
 * nor MBEDTLS_RSA_C nor any opaque signature mechanism (PSA or RSA_ALT).
 */
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE 0

#if (defined(MBEDTLS_RSA_C) || defined(MBEDTLS_PK_RSA_ALT_SUPPORT)) && \
    MBEDTLS_MPI_MAX_SIZE > MBEDTLS_PK_SIGNATURE_MAX_SIZE
/* For RSA, the signature can be as large as the bignum module allows.
 * For RSA_ALT, the signature size is not necessarily tied to what the
 * bignum module can do, but in the absence of any specific setting,
 * we use that (rsa_alt_sign_wrap in library/pk_wrap.h will check). */
#undef MBEDTLS_PK_SIGNATURE_MAX_SIZE
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE MBEDTLS_MPI_MAX_SIZE
#endif

#if defined(MBEDTLS_ECDSA_C) &&                                 \
    MBEDTLS_ECDSA_MAX_LEN > MBEDTLS_PK_SIGNATURE_MAX_SIZE
/* For ECDSA, the ecdsa module exports a constant for the maximum
 * signature size. */
#undef MBEDTLS_PK_SIGNATURE_MAX_SIZE
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE MBEDTLS_ECDSA_MAX_LEN
#endif

/* Internal helper to define which fields in the pk_context structure below
 * should be used for EC keys: legacy ecp_keypair or the raw (PSA friendly)
 * format. It should be noted that this only affects how data is stored, not
 * which functions are used for various operations. The overall picture looks
 * like this:
 * - if USE_PSA is not defined and ECP_C is defined then use ecp_keypair data
 *   structure and legacy functions
 * - if USE_PSA is defined and
 *     - if ECP_C then use ecp_keypair structure, convert data to a PSA friendly
 *       format and use PSA functions
 *     - if !ECP_C then use new raw data and PSA functions directly.
 *
 * The main reason for the "intermediate" (USE_PSA + ECP_C) above is that as long
 * as ECP_C is defined mbedtls_pk_ec() gives the user a read/write access to the
 * ecp_keypair structure inside the pk_context so they can modify it using
 * ECP functions which are not under PK module's control.
 */

/**
 * \brief           Types for interfacing with the debug module
 */
typedef enum {
    MBEDTLS_PK_DEBUG_NONE = 0,
    MBEDTLS_PK_DEBUG_MPI,
    MBEDTLS_PK_DEBUG_ECP,
    MBEDTLS_PK_DEBUG_PSA_EC,
} mbedtls_pk_debug_type;

/**
 * \brief           Item to send to the debug module
 */
typedef struct mbedtls_pk_debug_item {
    mbedtls_pk_debug_type MBEDTLS_PRIVATE(type);
    const char *MBEDTLS_PRIVATE(name);
    void *MBEDTLS_PRIVATE(value);
} mbedtls_pk_debug_item;

/** Maximum number of item send for debugging, plus 1 */
#define MBEDTLS_PK_DEBUG_MAX_ITEMS 3

/**
 * \brief           Public key information and operations
 *
 * \note        The library does not support custom pk info structures,
 *              only built-in structures returned by
 *              mbedtls_cipher_info_from_type().
 */
typedef struct mbedtls_pk_info_t mbedtls_pk_info_t;

#define MBEDTLS_PK_MAX_EC_PUBKEY_RAW_LEN \
    PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)
/**
 * \brief           Public key container
 */
typedef struct mbedtls_pk_context {
    /** Method table */
    const mbedtls_pk_info_t *MBEDTLS_PRIVATE(pk_info);
    /** Underlying type-specific key context */
    void *MBEDTLS_PRIVATE(pk_ctx);

    /* The following field is used to store the ID of a private key in the
     * following cases:
     * - opaque key when MBEDTLS_USE_PSA_CRYPTO is defined
     * - normal key when MBEDTLS_PK_USE_PSA_EC_DATA is defined. In this case:
     *    - the pk_ctx above is not not used to store the private key anymore.
     *      Actually that field not populated at all in this case because also
     *      the public key will be stored in raw format as explained below
     *    - this ID is used for all private key operations (ex: sign, check
     *      key pair, key write, etc) using PSA functions
     *
     * Note: this private key storing solution only affects EC keys, not the
     *       other ones. The latters still use the pk_ctx to store their own
     *       context. */
    /* The following fields are meant for storing the public key in raw format
     * which is handy for:
     * - easily importing it into the PSA context
     * - reducing the ECP module dependencies in the PK one.
     *
     * When MBEDTLS_PK_USE_PSA_EC_DATA is enabled:
     * - the pk_ctx above is not used anymore for storing the public key
     *   inside the ecp_keypair structure
     * - the following fields are used for all public key operations: signature
     *   verify, key pair check and key write.
     * - For a key pair, priv_id contains the private key. For a public key,
     *   priv_id is null.
     * Of course, when MBEDTLS_PK_USE_PSA_EC_DATA is not enabled, the legacy
     * ecp_keypair structure is used for storing the public key and performing
     * all the operations.
     *
     * Note: This new public key storing solution only works for EC keys, not
     *       other ones. The latters still use pk_ctx to store their own
     *       context.
     */
} mbedtls_pk_context;

/* Now we can declare functions that take a pointer to that */
typedef void mbedtls_pk_restart_ctx;

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/**
 * \brief           Types for RSA-alt abstraction
 */
typedef int (*mbedtls_pk_rsa_alt_decrypt_func)(void *ctx, size_t *olen,
                                               const unsigned char *input, unsigned char *output,
                                               size_t output_max_len, MinTlsDiagnostics* diagnostics);
typedef int (*mbedtls_pk_rsa_alt_sign_func)(void *ctx,
                                            mbedtls_f_rng_t *f_rng,
                                            void *p_rng,
                                            mbedtls_md_type_t md_alg, unsigned int hashlen,
                                            const unsigned char *hash, unsigned char *sig, MinTlsDiagnostics* diagnostics);
typedef size_t (*mbedtls_pk_rsa_alt_key_len_func)(void *ctx);
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/**
 * \brief           Return information associated with the given PK type
 *
 * \param pk_type   PK type to search for.
 *
 * \return          The PK info associated with the type or NULL if not found.
 */
const mbedtls_pk_info_t *mbedtls_pk_info_from_type(mbedtls_pk_type_t pk_type);

/**
 * \brief           Free the components of a #mbedtls_pk_context.
 *
 * \param ctx       The context to clear. It must have been initialized.
 *                  If this is \c NULL, this function does nothing.
 *
 * \note            For contexts that have been set up with
 *                  mbedtls_pk_setup_opaque(), this does not free the underlying
 *                  PSA key and you still need to call psa_destroy_key()
 *                  independently if you want to destroy that key.
 */
void mbedtls_pk_free(mbedtls_pk_context *ctx);

/**
 * \brief           Initialize a PK context with the information given
 *                  and allocates the type-specific PK subcontext.
 *
 * \param ctx       Context to initialize. It must not have been set
 *                  up yet (type #MBEDTLS_PK_NONE).
 * \param info      Information to use
 *
 * \return          0 on success,
 *                  MBEDTLS_ERR_PK_BAD_INPUT_DATA on invalid input,
 *                  MBEDTLS_ERR_PK_ALLOC_FAILED on allocation failure.
 *
 * \note            For contexts holding an RSA-alt key, use
 *                  \c mbedtls_pk_setup_rsa_alt() instead.
 */
int mbedtls_pk_setup(mbedtls_pk_context *ctx, const mbedtls_pk_info_t *info, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)

#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/**
 * \brief           Get the size in bits of the underlying key
 *
 * \param ctx       The context to query. It must have been initialized.
 *
 * \return          Key size in bits, or 0 on error
 */
size_t mbedtls_pk_get_bitlen(const mbedtls_pk_context *ctx);

/**
 * \brief           Get the length in bytes of the underlying key
 *
 * \param ctx       The context to query. It must have been initialized.
 *
 * \return          Key length in bytes, or 0 on error
 */
static inline size_t mbedtls_pk_get_len(const mbedtls_pk_context *ctx)
{
    return (mbedtls_pk_get_bitlen(ctx) + 7) / 8;
}

/**
 * \brief           Tell if a context can do the operation given by type
 *
 * \param ctx       The context to query. It must have been initialized.
 * \param type      The desired type.
 *
 * \return          1 if the context can do operations on the given type.
 * \return          0 if the context cannot do the operations on the given
 *                  type. This is always the case for a context that has
 *                  been initialized but not set up, or that has been
 *                  cleared with mbedtls_pk_free().
 */
int mbedtls_pk_can_do(const mbedtls_pk_context *ctx, mbedtls_pk_type_t type);

/**
 * \brief           Verify signature (including padding if relevant).
 *
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used.
 *                  This can be #MBEDTLS_MD_NONE if the signature algorithm
 *                  does not rely on a hash algorithm (non-deterministic
 *                  ECDSA, RSA PKCS#1 v1.5).
 *                  For PKCS#1 v1.5, if \p md_alg is #MBEDTLS_MD_NONE, then
 *                  \p hash is the DigestInfo structure used by RFC 8017
 *                  &sect;9.2 steps 3&ndash;6. If \p md_alg is a valid hash
 *                  algorithm then \p hash is the digest itself, and this
 *                  function calculates the DigestInfo encoding internally.
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 *
 * \note            For keys of type #MBEDTLS_PK_RSA, the signature algorithm is
 *                  either PKCS#1 v1.5 or PSS (accepting any salt length),
 *                  depending on the padding mode in the underlying RSA context.
 *                  For a pk object constructed by parsing, this is PKCS#1 v1.5
 *                  by default. Use mbedtls_pk_verify_ext() to explicitly select
 *                  a different algorithm.
 *
 * \return          0 on success (signature is valid),
 *                  #MBEDTLS_ERR_PK_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig but its length is less than \p sig_len,
 *                  or a specific error code.
 */
int mbedtls_pk_verify(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                      const unsigned char *hash, size_t hash_len,
                      const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Restartable version of \c mbedtls_pk_verify()
 *
 * \note            Performs the same job as \c mbedtls_pk_verify(), but can
 *                  return early and restart according to the limit set with
 *                  \c mbedtls_ecp_set_max_ops() to reduce blocking for ECC
 *                  operations. For RSA, same as \c mbedtls_pk_verify().
 *
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length or 0 (see notes)
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 * \param rs_ctx    Restart context (NULL to disable restart)
 *
 * \return          See \c mbedtls_pk_verify(), or
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 */
int mbedtls_pk_verify_restartable(mbedtls_pk_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hash_len,
                                  const unsigned char *sig, size_t sig_len,
                                  mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Verify signature, with options.
 *                  (Includes verification of the padding depending on type.)
 *
 * \param type      Signature type (inc. possible padding type) to verify
 * \param options   Pointer to type-specific options, or NULL
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length or 0 (see notes)
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 *
 * \return          0 on success (signature is valid),
 *                  #MBEDTLS_ERR_PK_TYPE_MISMATCH if the PK context can't be
 *                  used for this type of signatures,
 *                  #MBEDTLS_ERR_PK_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig but its length is less than \p sig_len,
 *                  or a specific error code.
 *
 * \note            If hash_len is 0, then the length associated with md_alg
 *                  is used instead, or an error returned if it is invalid.
 *
 * \note            md_alg may be MBEDTLS_MD_NONE, only if hash_len != 0
 *
 * \note            If type is MBEDTLS_PK_RSASSA_PSS, then options must point
 *                  to a mbedtls_pk_rsassa_pss_options structure,
 *                  otherwise it must be NULL. Note that if
 *                  #MBEDTLS_USE_PSA_CRYPTO is defined, the salt length is not
 *                  verified as PSA_ALG_RSA_PSS_ANY_SALT is used.
 */
int mbedtls_pk_verify_ext(mbedtls_pk_type_t type, const void *options,
                          mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                          const unsigned char *hash, size_t hash_len,
                          const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Restartable version of \c mbedtls_pk_sign()
 *
 * \note            Performs the same job as \c mbedtls_pk_sign(), but can
 *                  return early and restart according to the limit set with
 *                  \c mbedtls_ecp_set_max_ops() to reduce blocking for ECC
 *                  operations. For RSA, same as \c mbedtls_pk_sign().
 *
 * \param ctx       The PK context to use. It must have been set up
 *                  with a private key.
 * \param md_alg    Hash algorithm used (see notes for mbedtls_pk_sign())
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Place to write the signature.
 *                  It must have enough room for the signature.
 *                  #MBEDTLS_PK_SIGNATURE_MAX_SIZE is always enough.
 *                  You may use a smaller buffer if it is large enough
 *                  given the key type.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param sig_len   On successful return,
 *                  the number of bytes written to \p sig.
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 * \param rs_ctx    Restart context (NULL to disable restart)
 *
 * \return          See \c mbedtls_pk_sign().
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 */
int mbedtls_pk_sign_restartable(mbedtls_pk_context *ctx,
                                mbedtls_md_type_t md_alg,
                                const unsigned char *hash, size_t hash_len,
                                unsigned char *sig, size_t sig_size, size_t *sig_len,
                                mbedtls_f_rng_t *f_rng, void *p_rng,
                                mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Get the key type
 *
 * \param ctx       The PK context to use. It must have been initialized.
 *
 * \return          Type on success.
 * \return          #MBEDTLS_PK_NONE for a context that has not been set up.
 */
mbedtls_pk_type_t mbedtls_pk_get_type(const mbedtls_pk_context *ctx);

/**
 * Quick access to an RSA context inside a PK context.
 *
 * \warning This function can only be used when the type of the context, as
 * returned by mbedtls_pk_get_type(), is #MBEDTLS_PK_RSA.
 * Ensuring that is the caller's responsibility.
 * Alternatively, you can check whether this function returns NULL.
 *
 * \return The internal RSA context held by the PK context, or NULL.
 */
static inline mbedtls_rsa_context *mbedtls_pk_rsa(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_RSA:
            return (mbedtls_rsa_context *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

/*
 * WARNING: Low-level functions. You probably do not want to use these unless
 *          you are certain you do ;)
 */

/**
 * \brief           Parse a SubjectPublicKeyInfo DER structure
 *
 * \param p         the position in the ASN.1 data
 * \param end       end of the buffer
 * \param pk        The PK context to fill. It must have been initialized
 *                  but not set up.
 *
 * \return          0 if successful, or a specific PK error code
 */
int mbedtls_pk_parse_subpubkey(unsigned char **p, const unsigned char *end,
                               mbedtls_pk_context *pk, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_PK_H */


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Supported ciphersuites (Official IANA names)
 */
#define MBEDTLS_TLS_RSA_WITH_NULL_MD5                    0x01   /**< Weak! */
#define MBEDTLS_TLS_RSA_WITH_NULL_SHA                    0x02   /**< Weak! */

#define MBEDTLS_TLS_PSK_WITH_NULL_SHA                    0x2C   /**< Weak! */
#define MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA                0x2D   /**< Weak! */
#define MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA                0x2E   /**< Weak! */
#define MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA             0x2F

#define MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA         0x33
#define MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA             0x35
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA         0x39

#define MBEDTLS_TLS_RSA_WITH_NULL_SHA256                 0x3B   /**< Weak! */
#define MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256          0x3C   /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256          0x3D   /**< TLS 1.2 */

#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA        0x41
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA    0x45

#define MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256      0x67   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256      0x6B   /**< TLS 1.2 */

#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA        0x84
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA    0x88

#define MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA             0x8C
#define MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA             0x8D

#define MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA         0x90
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA         0x91

#define MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA         0x94
#define MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA         0x95

#define MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256          0x9C   /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384          0x9D   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256      0x9E   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384      0x9F   /**< TLS 1.2 */

#define MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256          0xA8   /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384          0xA9   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_128_GCM_SHA256      0xAA   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_256_GCM_SHA384      0xAB   /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_AES_128_GCM_SHA256      0xAC   /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_AES_256_GCM_SHA384      0xAD   /**< TLS 1.2 */

#define MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA256          0xAE
#define MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384          0xAF
#define MBEDTLS_TLS_PSK_WITH_NULL_SHA256                 0xB0   /**< Weak! */
#define MBEDTLS_TLS_PSK_WITH_NULL_SHA384                 0xB1   /**< Weak! */

#define MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA256      0xB2
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA384      0xB3
#define MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA256             0xB4   /**< Weak! */
#define MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA384             0xB5   /**< Weak! */

#define MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA256      0xB6
#define MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA384      0xB7
#define MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA256             0xB8   /**< Weak! */
#define MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA384             0xB9   /**< Weak! */

#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256     0xBA   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256 0xBE   /**< TLS 1.2 */

#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256     0xC0   /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256 0xC4   /**< TLS 1.2 */

#define MBEDTLS_TLS_ECDH_ECDSA_WITH_NULL_SHA             0xC001 /**< Weak! */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA      0xC004
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA      0xC005

#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_NULL_SHA            0xC006 /**< Weak! */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA     0xC009
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA     0xC00A

#define MBEDTLS_TLS_ECDH_RSA_WITH_NULL_SHA               0xC00B /**< Weak! */
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA        0xC00E
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA        0xC00F

#define MBEDTLS_TLS_ECDHE_RSA_WITH_NULL_SHA              0xC010 /**< Weak! */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA       0xC013
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA       0xC014

#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256  0xC023 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384  0xC024 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256   0xC025 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384   0xC026 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256    0xC027 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384    0xC028 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256     0xC029 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384     0xC02A /**< TLS 1.2 */

#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256  0xC02B /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384  0xC02C /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256   0xC02D /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384   0xC02E /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256    0xC02F /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384    0xC030 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256     0xC031 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384     0xC032 /**< TLS 1.2 */

#define MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA       0xC035
#define MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA       0xC036
#define MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256    0xC037
#define MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384    0xC038
#define MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA              0xC039
#define MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA256           0xC03A
#define MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA384           0xC03B

#define MBEDTLS_TLS_RSA_WITH_ARIA_128_CBC_SHA256         0xC03C /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_ARIA_256_CBC_SHA384         0xC03D /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_ARIA_128_CBC_SHA256     0xC044 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_ARIA_256_CBC_SHA384     0xC045 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256 0xC048 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384 0xC049 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256  0xC04A /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384  0xC04B /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256   0xC04C /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384   0xC04D /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_ARIA_128_CBC_SHA256    0xC04E /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_ARIA_256_CBC_SHA384    0xC04F /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_ARIA_128_GCM_SHA256         0xC050 /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_ARIA_256_GCM_SHA384         0xC051 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_ARIA_128_GCM_SHA256     0xC052 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_ARIA_256_GCM_SHA384     0xC053 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256 0xC05C /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384 0xC05D /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256  0xC05E /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384  0xC05F /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256   0xC060 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384   0xC061 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_ARIA_128_GCM_SHA256    0xC062 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_ARIA_256_GCM_SHA384    0xC063 /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_ARIA_128_CBC_SHA256         0xC064 /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_ARIA_256_CBC_SHA384         0xC065 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_ARIA_128_CBC_SHA256     0xC066 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_ARIA_256_CBC_SHA384     0xC067 /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_ARIA_128_CBC_SHA256     0xC068 /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_ARIA_256_CBC_SHA384     0xC069 /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_ARIA_128_GCM_SHA256         0xC06A /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_ARIA_256_GCM_SHA384         0xC06B /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_ARIA_128_GCM_SHA256     0xC06C /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_ARIA_256_GCM_SHA384     0xC06D /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_ARIA_128_GCM_SHA256     0xC06E /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_ARIA_256_GCM_SHA384     0xC06F /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_PSK_WITH_ARIA_128_CBC_SHA256   0xC070 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_PSK_WITH_ARIA_256_CBC_SHA384   0xC071 /**< TLS 1.2 */

#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256 0xC072
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384 0xC073
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256  0xC074
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384  0xC075
#define MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256   0xC076
#define MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384   0xC077
#define MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256    0xC078
#define MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384    0xC079

#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256         0xC07A /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384         0xC07B /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256     0xC07C /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384     0xC07D /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256 0xC086 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384 0xC087 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256  0xC088 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384  0xC089 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256   0xC08A /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384   0xC08B /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256    0xC08C /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384    0xC08D /**< TLS 1.2 */

#define MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256       0xC08E /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384       0xC08F /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256   0xC090 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384   0xC091 /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256   0xC092 /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384   0xC093 /**< TLS 1.2 */

#define MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256       0xC094
#define MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384       0xC095
#define MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256   0xC096
#define MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384   0xC097
#define MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256   0xC098
#define MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384   0xC099
#define MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256 0xC09A
#define MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384 0xC09B

#define MBEDTLS_TLS_RSA_WITH_AES_128_CCM                0xC09C  /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_AES_256_CCM                0xC09D  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM            0xC09E  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM            0xC09F  /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_AES_128_CCM_8              0xC0A0  /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_WITH_AES_256_CCM_8              0xC0A1  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM_8          0xC0A2  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM_8          0xC0A3  /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_AES_128_CCM                0xC0A4  /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_AES_256_CCM                0xC0A5  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM            0xC0A6  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM            0xC0A7  /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8              0xC0A8  /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_AES_256_CCM_8              0xC0A9  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM_8          0xC0AA  /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM_8          0xC0AB  /**< TLS 1.2 */
/* The last two are named with PSK_DHE in the RFC, which looks like a typo */

#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM        0xC0AC  /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM        0xC0AD  /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8      0xC0AE  /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8      0xC0AF  /**< TLS 1.2 */

#define MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8          0xC0FF  /**< experimental */

/* RFC 7905 */
#define MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256   0xCCA8 /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 0xCCA9 /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256     0xCCAA /**< TLS 1.2 */
#define MBEDTLS_TLS_PSK_WITH_CHACHA20_POLY1305_SHA256         0xCCAB /**< TLS 1.2 */
#define MBEDTLS_TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256   0xCCAC /**< TLS 1.2 */
#define MBEDTLS_TLS_DHE_PSK_WITH_CHACHA20_POLY1305_SHA256     0xCCAD /**< TLS 1.2 */
#define MBEDTLS_TLS_RSA_PSK_WITH_CHACHA20_POLY1305_SHA256     0xCCAE /**< TLS 1.2 */

/* RFC 8446, Appendix B.4 */
#define MBEDTLS_TLS1_3_AES_128_GCM_SHA256                     0x1301 /**< TLS 1.3 */
#define MBEDTLS_TLS1_3_AES_256_GCM_SHA384                     0x1302 /**< TLS 1.3 */
#define MBEDTLS_TLS1_3_CHACHA20_POLY1305_SHA256               0x1303 /**< TLS 1.3 */
#define MBEDTLS_TLS1_3_AES_128_CCM_SHA256                     0x1304 /**< TLS 1.3 */
#define MBEDTLS_TLS1_3_AES_128_CCM_8_SHA256                   0x1305 /**< TLS 1.3 */

/* Reminder: update mbedtls_ssl_premaster_secret when adding a new key exchange.
 * Reminder: update MBEDTLS_KEY_EXCHANGE__xxx below
 */
typedef enum {
    MBEDTLS_KEY_EXCHANGE_NONE = 0,
    MBEDTLS_KEY_EXCHANGE_RSA,
    MBEDTLS_KEY_EXCHANGE_DHE_RSA,
    MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,
    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,
    MBEDTLS_KEY_EXCHANGE_PSK,
    MBEDTLS_KEY_EXCHANGE_DHE_PSK,
    MBEDTLS_KEY_EXCHANGE_RSA_PSK,
    MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,
    MBEDTLS_KEY_EXCHANGE_ECDH_RSA,
    MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,
    MBEDTLS_KEY_EXCHANGE_ECJPAKE,
} mbedtls_key_exchange_type_t;

/* Key exchanges using a certificate */
#define MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED

/* Key exchanges in either TLS 1.2 or 1.3 which are using an ECDSA
 * signature */
#define MBEDTLS_KEY_EXCHANGE_WITH_ECDSA_ANY_ENABLED

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED) || \
    defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
#define MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED
#endif

/* Key exchanges allowing client certificate requests.
 *
 * Note: that's almost the same as MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED
 * above, except RSA-PSK uses a server certificate but no client cert.
 *
 * Note: this difference is specific to TLS 1.2, as with TLS 1.3, things are
 * more symmetrical: client certs and server certs are either both allowed
 * (Ephemeral mode) or both disallowed (PSK and PKS-Ephemeral modes).
 */
#define MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED

/* Helper to state that certificate-based client authentication through ECDSA
 * is supported in TLS 1.2 */
#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED) && \
    defined(MBEDTLS_PK_CAN_ECDSA_SIGN) && defined(MBEDTLS_PK_CAN_ECDSA_VERIFY)
#define MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED
#endif

/* ECDSA required for certificates in either TLS 1.2 or 1.3 */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
    defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
#define MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED
#endif

/* Key exchanges involving server signature in ServerKeyExchange */
#define MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED

/* Key exchanges using ECDH */

/* Key exchanges that don't involve ephemeral keys */

/* Key exchanges that involve ephemeral keys */
#define MBEDTLS_KEY_EXCHANGE_SOME_PFS_ENABLED

/* Key exchanges using a PSK */

/* Key exchanges using DHE */

/* Key exchanges using ECDHE */
#define MBEDTLS_KEY_EXCHANGE_SOME_ECDHE_ENABLED

/* TLS 1.2 key exchanges using ECDH or ECDHE*/
#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDHE_ENABLED)
#define MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED
#endif

/* TLS 1.3 PSK key exchanges */

/* TLS 1.2 or 1.3 key exchanges with PSK */

/* TLS 1.3 ephemeral key exchanges */

/* TLS 1.3 key exchanges using ECDHE */

/* TLS 1.2 or 1.3 key exchanges using ECDH or ECDHE */
#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_ECDHE_ENABLED)
#define MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED
#endif

/* TLS 1.2 XXDH key exchanges: ECDH or ECDHE or FFDH */
#if (defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_DHE_ENABLED))
#define MBEDTLS_KEY_EXCHANGE_SOME_XXDH_1_2_ENABLED
#endif

/* The handshake params structure has a set of fields called xxdh_psa which are used:
 * - by TLS 1.2 with `USE_PSA` to do ECDH or ECDHE;
 * - by TLS 1.3 to do ECDHE or FFDHE.
 * The following macros can be used to guard their declaration and use.
 */

typedef struct mbedtls_ssl_ciphersuite_t mbedtls_ssl_ciphersuite_t;

#define MBEDTLS_CIPHERSUITE_WEAK       0x01    /**< Weak ciphersuite flag  */
#define MBEDTLS_CIPHERSUITE_SHORT_TAG  0x02    /**< Short authentication tag,
                                                     eg for CCM_8 */
#define MBEDTLS_CIPHERSUITE_NODTLS     0x04    /**< Can't be used with DTLS */

/**
 * \brief   This structure is used for storing ciphersuite information
 *
 * \note    members are defined using integral types instead of enums
 *          in order to pack structure and reduce memory usage by internal
 *          \c ciphersuite_definitions[]
 */
struct mbedtls_ssl_ciphersuite_t {
    int MBEDTLS_PRIVATE(id);
    const char *MBEDTLS_PRIVATE(name);

    uint8_t MBEDTLS_PRIVATE(cipher);           /* mbedtls_cipher_type_t */
    uint8_t MBEDTLS_PRIVATE(mac);              /* mbedtls_md_type_t */
    uint8_t MBEDTLS_PRIVATE(key_exchange);     /* mbedtls_key_exchange_type_t */
    uint8_t MBEDTLS_PRIVATE(flags);

    uint16_t MBEDTLS_PRIVATE(min_tls_version); /* mbedtls_ssl_protocol_version */
    uint16_t MBEDTLS_PRIVATE(max_tls_version); /* mbedtls_ssl_protocol_version */
};

const int *mbedtls_ssl_list_ciphersuites(void);

const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_id(int ciphersuite_id);

#ifdef __cplusplus
}
#endif

#endif /* ssl_ciphersuites.h */


/* ===== Retained x509_crt.h ===== */

/**
 * \file x509_crt.h
 *
 * \brief X.509 certificate parsing and writing
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/x509_crt.h.
 */
#ifndef MBEDTLS_X509_CRT_H
#define MBEDTLS_X509_CRT_H


/* ===== Retained x509.h ===== */

/**
 * \file x509.h
 *
 * \brief X.509 generic defines and structures
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/x509.h.
 */
#ifndef MBEDTLS_X509_H
#define MBEDTLS_X509_H




/**
 * \addtogroup x509_module
 * \{
 */

#if !defined(MBEDTLS_X509_MAX_INTERMEDIATE_CA)
/**
 * Maximum number of intermediate CAs in a verification chain.
 * That is, maximum length of the chain, excluding the end-entity certificate
 * and the trusted root certificate.
 *
 * Set this to a low value to prevent an adversary from making you waste
 * resources verifying an overlong certificate chain.
 */
#define MBEDTLS_X509_MAX_INTERMEDIATE_CA   8
#endif

/**
 * \name X509 Error codes
 * \{
 */
/** Unavailable feature, e.g. RSA hashing/encryption combination. */
#define MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE              -0x2080
/** Requested OID is unknown. */
#define MBEDTLS_ERR_X509_UNKNOWN_OID                      -0x2100
/** The CRT/CRL/CSR format is invalid, e.g. different type expected. */
#define MBEDTLS_ERR_X509_INVALID_FORMAT                   -0x2180
/** The CRT/CRL/CSR version element is invalid. */
#define MBEDTLS_ERR_X509_INVALID_VERSION                  -0x2200
/** The serial tag or value is invalid. */
#define MBEDTLS_ERR_X509_INVALID_SERIAL                   -0x2280
/** The algorithm tag or value is invalid. */
#define MBEDTLS_ERR_X509_INVALID_ALG                      -0x2300
/** The name tag or value is invalid. */
#define MBEDTLS_ERR_X509_INVALID_NAME                     -0x2380
/** The date tag or value is invalid. */
#define MBEDTLS_ERR_X509_INVALID_DATE                     -0x2400
/** The signature tag or value invalid. */
#define MBEDTLS_ERR_X509_INVALID_SIGNATURE                -0x2480
/** The extension tag or value is invalid. */
#define MBEDTLS_ERR_X509_INVALID_EXTENSIONS               -0x2500
/** CRT/CRL/CSR has an unsupported version number. */
#define MBEDTLS_ERR_X509_UNKNOWN_VERSION                  -0x2580
/** Signature algorithm (oid) is unsupported. */
#define MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG                  -0x2600
/** Signature algorithms do not match. (see \c ::mbedtls_x509_crt sig_oid) */
#define MBEDTLS_ERR_X509_SIG_MISMATCH                     -0x2680
/** Certificate verification failed, e.g. CRL, CA or signature check failed. */
#define MBEDTLS_ERR_X509_CERT_VERIFY_FAILED               -0x2700
/** Format not recognized as DER or PEM. */
#define MBEDTLS_ERR_X509_CERT_UNKNOWN_FORMAT              -0x2780
/** Input invalid. */
#define MBEDTLS_ERR_X509_BAD_INPUT_DATA                   -0x2800
/** Allocation of memory failed. */
#define MBEDTLS_ERR_X509_ALLOC_FAILED                     -0x2880
/** Read/write of file failed. */
#define MBEDTLS_ERR_X509_FILE_IO_ERROR                    -0x2900
/** Destination buffer is too small. */
#define MBEDTLS_ERR_X509_BUFFER_TOO_SMALL                 -0x2980
/** A fatal error occurred, eg the chain is too long or the vrfy callback failed. */
#define MBEDTLS_ERR_X509_FATAL_ERROR                      -0x3000
/** \} name X509 Error codes */

/**
 * \name X509 Verify codes
 * \{
 */
/* Reminder: update x509_crt_verify_strings[] in library/x509_crt.c */
#define MBEDTLS_X509_BADCERT_EXPIRED             0x01  /**< The certificate validity has expired. */
#define MBEDTLS_X509_BADCERT_REVOKED             0x02  /**< The certificate has been revoked (is on a CRL). */
#define MBEDTLS_X509_BADCERT_CN_MISMATCH         0x04  /**< The certificate Common Name (CN) does not match with the expected CN. */
#define MBEDTLS_X509_BADCERT_NOT_TRUSTED         0x08  /**< The certificate is not correctly signed by the trusted CA. */
#define MBEDTLS_X509_BADCRL_NOT_TRUSTED          0x10  /**< The CRL is not correctly signed by the trusted CA. */
#define MBEDTLS_X509_BADCRL_EXPIRED              0x20  /**< The CRL is expired. */
#define MBEDTLS_X509_BADCERT_MISSING             0x40  /**< Certificate was missing. */
#define MBEDTLS_X509_BADCERT_SKIP_VERIFY         0x80  /**< Certificate verification was skipped. */
#define MBEDTLS_X509_BADCERT_OTHER             0x0100  /**< Other reason (can be used by verify callback) */
#define MBEDTLS_X509_BADCERT_FUTURE            0x0200  /**< The certificate validity starts in the future. */
#define MBEDTLS_X509_BADCRL_FUTURE             0x0400  /**< The CRL is from the future */
#define MBEDTLS_X509_BADCERT_KEY_USAGE         0x0800  /**< Usage does not match the keyUsage extension. */
#define MBEDTLS_X509_BADCERT_EXT_KEY_USAGE     0x1000  /**< Usage does not match the extendedKeyUsage extension. */
#define MBEDTLS_X509_BADCERT_NS_CERT_TYPE      0x2000  /**< Usage does not match the nsCertType extension. */
#define MBEDTLS_X509_BADCERT_BAD_MD            0x4000  /**< The certificate is signed with an unacceptable hash. */
#define MBEDTLS_X509_BADCERT_BAD_PK            0x8000  /**< The certificate is signed with an unacceptable PK alg (eg RSA vs ECDSA). */
#define MBEDTLS_X509_BADCERT_BAD_KEY         0x010000  /**< The certificate is signed with an unacceptable key (eg bad curve, RSA too short). */
#define MBEDTLS_X509_BADCRL_BAD_MD           0x020000  /**< The CRL is signed with an unacceptable hash. */
#define MBEDTLS_X509_BADCRL_BAD_PK           0x040000  /**< The CRL is signed with an unacceptable PK alg (eg RSA vs ECDSA). */
#define MBEDTLS_X509_BADCRL_BAD_KEY          0x080000  /**< The CRL is signed with an unacceptable key (eg bad curve, RSA too short). */

/** \} name X509 Verify codes */
/** \} addtogroup x509_module */

/*
 * X.509 v3 Subject Alternative Name types.
 *      otherName                       [0]     OtherName,
 *      rfc822Name                      [1]     IA5String,
 *      dNSName                         [2]     IA5String,
 *      x400Address                     [3]     ORAddress,
 *      directoryName                   [4]     Name,
 *      ediPartyName                    [5]     EDIPartyName,
 *      uniformResourceIdentifier       [6]     IA5String,
 *      iPAddress                       [7]     OCTET STRING,
 *      registeredID                    [8]     OBJECT IDENTIFIER
 */
#define MBEDTLS_X509_SAN_OTHER_NAME                      0
#define MBEDTLS_X509_SAN_RFC822_NAME                     1
#define MBEDTLS_X509_SAN_DNS_NAME                        2
#define MBEDTLS_X509_SAN_X400_ADDRESS_NAME               3
#define MBEDTLS_X509_SAN_DIRECTORY_NAME                  4
#define MBEDTLS_X509_SAN_EDI_PARTY_NAME                  5
#define MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER     6
#define MBEDTLS_X509_SAN_IP_ADDRESS                      7
#define MBEDTLS_X509_SAN_REGISTERED_ID                   8

/*
 * X.509 v3 Key Usage Extension flags
 * Reminder: update mbedtls_x509_info_key_usage() when adding new flags.
 */
#define MBEDTLS_X509_KU_DIGITAL_SIGNATURE            (0x80)  /* bit 0 */
#define MBEDTLS_X509_KU_NON_REPUDIATION              (0x40)  /* bit 1 */
#define MBEDTLS_X509_KU_KEY_ENCIPHERMENT             (0x20)  /* bit 2 */
#define MBEDTLS_X509_KU_DATA_ENCIPHERMENT            (0x10)  /* bit 3 */
#define MBEDTLS_X509_KU_KEY_AGREEMENT                (0x08)  /* bit 4 */
#define MBEDTLS_X509_KU_KEY_CERT_SIGN                (0x04)  /* bit 5 */
#define MBEDTLS_X509_KU_CRL_SIGN                     (0x02)  /* bit 6 */
#define MBEDTLS_X509_KU_ENCIPHER_ONLY                (0x01)  /* bit 7 */
#define MBEDTLS_X509_KU_DECIPHER_ONLY              (0x8000)  /* bit 8 */

/*
 * Netscape certificate types
 * (http://www.mozilla.org/projects/security/pki/nss/tech-notes/tn3.html)
 */

#define MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT         (0x80)  /* bit 0 */
#define MBEDTLS_X509_NS_CERT_TYPE_SSL_SERVER         (0x40)  /* bit 1 */
#define MBEDTLS_X509_NS_CERT_TYPE_EMAIL              (0x20)  /* bit 2 */
#define MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING     (0x10)  /* bit 3 */
#define MBEDTLS_X509_NS_CERT_TYPE_RESERVED           (0x08)  /* bit 4 */
#define MBEDTLS_X509_NS_CERT_TYPE_SSL_CA             (0x04)  /* bit 5 */
#define MBEDTLS_X509_NS_CERT_TYPE_EMAIL_CA           (0x02)  /* bit 6 */
#define MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING_CA  (0x01)  /* bit 7 */

/*
 * X.509 extension types
 *
 * Comments refer to the status for using certificates. Status can be
 * different for writing certificates or reading CRLs or CSRs.
 *
 * Those are defined in oid.h as oid.c needs them in a data structure. Since
 * these were previously defined here, let's have aliases for compatibility.
 */
#define MBEDTLS_X509_EXT_AUTHORITY_KEY_IDENTIFIER MBEDTLS_OID_X509_EXT_AUTHORITY_KEY_IDENTIFIER
#define MBEDTLS_X509_EXT_SUBJECT_KEY_IDENTIFIER   MBEDTLS_OID_X509_EXT_SUBJECT_KEY_IDENTIFIER
#define MBEDTLS_X509_EXT_KEY_USAGE                MBEDTLS_OID_X509_EXT_KEY_USAGE
#define MBEDTLS_X509_EXT_CERTIFICATE_POLICIES     MBEDTLS_OID_X509_EXT_CERTIFICATE_POLICIES
#define MBEDTLS_X509_EXT_POLICY_MAPPINGS          MBEDTLS_OID_X509_EXT_POLICY_MAPPINGS
#define MBEDTLS_X509_EXT_SUBJECT_ALT_NAME         MBEDTLS_OID_X509_EXT_SUBJECT_ALT_NAME         /* Supported (DNS) */
#define MBEDTLS_X509_EXT_ISSUER_ALT_NAME          MBEDTLS_OID_X509_EXT_ISSUER_ALT_NAME
#define MBEDTLS_X509_EXT_SUBJECT_DIRECTORY_ATTRS  MBEDTLS_OID_X509_EXT_SUBJECT_DIRECTORY_ATTRS
#define MBEDTLS_X509_EXT_BASIC_CONSTRAINTS        MBEDTLS_OID_X509_EXT_BASIC_CONSTRAINTS        /* Supported */
#define MBEDTLS_X509_EXT_NAME_CONSTRAINTS         MBEDTLS_OID_X509_EXT_NAME_CONSTRAINTS
#define MBEDTLS_X509_EXT_POLICY_CONSTRAINTS       MBEDTLS_OID_X509_EXT_POLICY_CONSTRAINTS
#define MBEDTLS_X509_EXT_EXTENDED_KEY_USAGE       MBEDTLS_OID_X509_EXT_EXTENDED_KEY_USAGE
#define MBEDTLS_X509_EXT_CRL_DISTRIBUTION_POINTS  MBEDTLS_OID_X509_EXT_CRL_DISTRIBUTION_POINTS
#define MBEDTLS_X509_EXT_INIHIBIT_ANYPOLICY       MBEDTLS_OID_X509_EXT_INIHIBIT_ANYPOLICY
#define MBEDTLS_X509_EXT_FRESHEST_CRL             MBEDTLS_OID_X509_EXT_FRESHEST_CRL
#define MBEDTLS_X509_EXT_NS_CERT_TYPE             MBEDTLS_OID_X509_EXT_NS_CERT_TYPE

/*
 * Storage format identifiers
 * Recognized formats: PEM and DER
 */
#define MBEDTLS_X509_FORMAT_DER                 1
#define MBEDTLS_X509_FORMAT_PEM                 2

#define MBEDTLS_X509_MAX_DN_NAME_SIZE         256 /**< Maximum value size of a DN entry */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup x509_module
 * \{ */

/**
 * \name Structures for parsing X.509 certificates, CRLs and CSRs
 * \{
 */

/**
 * Type-length-value structure that allows for ASN1 using DER.
 */
typedef mbedtls_asn1_buf mbedtls_x509_buf;

/**
 * Container for ASN1 bit strings.
 */
typedef mbedtls_asn1_bitstring mbedtls_x509_bitstring;

/**
 * Container for ASN1 named information objects.
 * It allows for Relative Distinguished Names (e.g. cn=localhost,ou=code,etc.).
 */
typedef mbedtls_asn1_named_data mbedtls_x509_name;

/**
 * Container for a sequence of ASN.1 items
 */
typedef mbedtls_asn1_sequence mbedtls_x509_sequence;

/*
 * Container for the fields of the Authority Key Identifier object
 */
typedef struct mbedtls_x509_authority {
    mbedtls_x509_buf keyIdentifier;
    mbedtls_x509_sequence authorityCertIssuer;
    mbedtls_x509_buf authorityCertSerialNumber;
    mbedtls_x509_buf raw;
}
mbedtls_x509_authority;

/** Container for date and time (precision in seconds). */
typedef struct mbedtls_x509_time {
    int year, mon, day;         /**< Date. */
    int hour, min, sec;         /**< Time. */
}
mbedtls_x509_time;

/**
 * From RFC 5280 section 4.2.1.6:
 * OtherName ::= SEQUENCE {
 *      type-id    OBJECT IDENTIFIER,
 *      value      [0] EXPLICIT ANY DEFINED BY type-id }
 *
 * Future versions of the library may add new fields to this structure or
 * to its embedded union and structure.
 */
typedef struct mbedtls_x509_san_other_name {
    /**
     * The type_id is an OID as defined in RFC 5280.
     * To check the value of the type id, you should use
     * \p MBEDTLS_OID_CMP with a known OID mbedtls_x509_buf.
     */
    mbedtls_x509_buf type_id;                   /**< The type id. */
    union {
        /**
         * From RFC 4108 section 5:
         * HardwareModuleName ::= SEQUENCE {
         *                         hwType OBJECT IDENTIFIER,
         *                         hwSerialNum OCTET STRING }
         */
        struct {
            mbedtls_x509_buf oid;               /**< The object identifier. */
            mbedtls_x509_buf val;               /**< The named value. */
        }
        hardware_module_name;
    }
    value;
}
mbedtls_x509_san_other_name;

/**
 * A structure for holding the parsed Subject Alternative Name,
 * according to type.
 *
 * Future versions of the library may add new fields to this structure or
 * to its embedded union and structure.
 */
typedef struct mbedtls_x509_subject_alternative_name {
    int type;                              /**< The SAN type, value of MBEDTLS_X509_SAN_XXX. */
    union {
        mbedtls_x509_san_other_name other_name;
        mbedtls_x509_name directory_name;
        mbedtls_x509_buf unstructured_name; /**< The buffer for the unstructured types. rfc822Name, dnsName and uniformResourceIdentifier are currently supported. */
    }
    san; /**< A union of the supported SAN types */
}
mbedtls_x509_subject_alternative_name;

typedef struct mbedtls_x509_san_list {
    mbedtls_x509_subject_alternative_name node;
    struct mbedtls_x509_san_list *next;
}
mbedtls_x509_san_list;

/**
 * \brief            Convert the certificate DN string \p name into
 *                   a linked list of mbedtls_x509_name (equivalent to
 *                   mbedtls_asn1_named_data).
 *
 * \note             This function allocates a linked list, and places the head
 *                   pointer in \p head. This list must later be freed by a
 *                   call to mbedtls_asn1_free_named_data_list().
 *
 * \param[out] head  Address in which to store the pointer to the head of the
 *                   allocated list of mbedtls_x509_name. Must point to NULL on
 *                   entry.
 * \param[in] name   The string representation of a DN to convert
 *
 * \return           0 on success, or a negative error code.
 */
int mbedtls_x509_string_to_names(mbedtls_asn1_named_data **head, const char *name);

/**
 * \brief          Compare pair of mbedtls_x509_time.
 *
 * \param t1       mbedtls_x509_time to compare
 * \param t2       mbedtls_x509_time to compare
 *
 * \return         < 0 if t1 is before t2
 *                   0 if t1 equals t2
 *                 > 0 if t1 is after t2
 */
int mbedtls_x509_time_cmp(const mbedtls_x509_time *t1, const mbedtls_x509_time *t2);

/**
 * \brief          Fill mbedtls_x509_time with provided mbedtls_time_t.
 *
 * \param tt       mbedtls_time_t to convert
 * \param now      mbedtls_x509_time to fill with converted mbedtls_time_t
 *
 * \return         \c 0 on success
 * \return         A non-zero return value on failure.
 */
int mbedtls_x509_time_gmtime(mbedtls_time_t tt, mbedtls_x509_time *now);

/**
 * \brief          This function parses an item in the SubjectAlternativeNames
 *                 extension. Please note that this function might allocate
 *                 additional memory for a subject alternative name, thus
 *                 mbedtls_x509_free_subject_alt_name has to be called
 *                 to dispose of this additional memory afterwards.
 *
 * \param san_buf  The buffer holding the raw data item of the subject
 *                 alternative name.
 * \param san      The target structure to populate with the parsed presentation
 *                 of the subject alternative name encoded in \p san_buf.
 *
 * \note           Supported GeneralName types, as defined in RFC 5280:
 *                 "rfc822Name", "dnsName", "directoryName",
 *                 "uniformResourceIdentifier" and "hardware_module_name"
 *                 of type "otherName", as defined in RFC 4108.
 *
 * \note           This function should be called on a single raw data of
 *                 subject alternative name. For example, after successful
 *                 certificate parsing, one must iterate on every item in the
 *                 \c crt->subject_alt_names sequence, and pass it to
 *                 this function.
 *
 * \warning        The target structure contains pointers to the raw data of the
 *                 parsed certificate, and its lifetime is restricted by the
 *                 lifetime of the certificate.
 *
 * \return         \c 0 on success
 * \return         #MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE for an unsupported
 *                 SAN type.
 * \return         Another negative value for any other failure.
 */
int mbedtls_x509_parse_subject_alt_name(const mbedtls_x509_buf *san_buf,
                                        mbedtls_x509_subject_alternative_name *san, MinTlsDiagnostics* diagnostics);
/**
 * \brief          Unallocate all data related to subject alternative name
 *
 * \param san      SAN structure - extra memory owned by this structure will be freed
 */
void mbedtls_x509_free_subject_alt_name(mbedtls_x509_subject_alternative_name *san);

/**
 * \brief          This function parses a CN string as an IP address.
 *
 * \param cn       The CN string to parse. CN string MUST be null-terminated.
 * \param dst      The target buffer to populate with the binary IP address.
 *                 The buffer MUST be 16 bytes to save IPv6, and should be
 *                 4-byte aligned if the result will be used as struct in_addr.
 *                 e.g. uint32_t dst[4]
 *
 * \note           \p cn is parsed as an IPv6 address if string contains ':',
 *                 else \p cn is parsed as an IPv4 address.
 *
 * \return         Length of binary IP address; num bytes written to target.
 * \return         \c 0 on failure to parse CN string as an IP address.
 */
size_t mbedtls_x509_crt_parse_cn_inet_pton(const char *cn, void *dst);

#define MBEDTLS_X509_SAFE_SNPRINTF                          \
    do {                                                    \
        if (ret < 0 || (size_t) ret >= n)                  \
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL);    \
                                                          \
        n -= (size_t) ret;                                  \
        p += (size_t) ret;                                  \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_X509_H */

/* ===== Retained x509_crl.h ===== */

/**
 * \file x509_crl.h
 *
 * \brief X.509 certificate revocation list parsing
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/x509_crl.h.
 */
#ifndef MBEDTLS_X509_CRL_H
#define MBEDTLS_X509_CRL_H



#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup x509_module
 * \{ */

/**
 * \name Structures and functions for parsing CRLs
 * \{
 */

/**
 * Certificate revocation list entry.
 * Contains the CA-specific serial numbers and revocation dates.
 *
 * Some fields of this structure are publicly readable. Do not modify
 * them except via Mbed TLS library functions: the effect of modifying
 * those fields or the data that those fields points to is unspecified.
 */
typedef struct mbedtls_x509_crl_entry {
    /** Direct access to the whole entry inside the containing buffer. */
    mbedtls_x509_buf raw;
    /** The serial number of the revoked certificate. */
    mbedtls_x509_buf serial;
    /** The revocation date of this entry. */
    mbedtls_x509_time revocation_date;
    /** Direct access to the list of CRL entry extensions
     * (an ASN.1 constructed sequence).
     *
     * If there are no extensions, `entry_ext.len == 0` and
     * `entry_ext.p == NULL`. */
    mbedtls_x509_buf entry_ext;

    /** Next element in the linked list of entries.
     * \p NULL indicates the end of the list.
     * Do not modify this field directly. */
    struct mbedtls_x509_crl_entry *next;
}
mbedtls_x509_crl_entry;

/**
 * Certificate revocation list structure.
 * Every CRL may have multiple entries.
 */
typedef struct mbedtls_x509_crl {
    mbedtls_x509_buf raw;           /**< The raw certificate data (DER). */
    mbedtls_x509_buf tbs;           /**< The raw certificate body (DER). The part that is To Be Signed. */

    int version;            /**< CRL version (1=v1, 2=v2) */
    mbedtls_x509_buf sig_oid;       /**< CRL signature type identifier */

    mbedtls_x509_buf issuer_raw;    /**< The raw issuer data (DER). */

    mbedtls_x509_name issuer;       /**< The parsed issuer data (named information object). */

    mbedtls_x509_time this_update;
    mbedtls_x509_time next_update;

    mbedtls_x509_crl_entry entry;   /**< The CRL entries containing the certificate revocation times for this CA. */

    mbedtls_x509_buf crl_ext;

    mbedtls_x509_buf MBEDTLS_PRIVATE(sig_oid2);
    mbedtls_x509_buf MBEDTLS_PRIVATE(sig);
    mbedtls_md_type_t MBEDTLS_PRIVATE(sig_md);           /**< Internal representation of the MD algorithm of the signature algorithm, e.g. MBEDTLS_MD_SHA256 */
    mbedtls_pk_type_t MBEDTLS_PRIVATE(sig_pk);           /**< Internal representation of the Public Key algorithm of the signature algorithm, e.g. MBEDTLS_PK_RSA */
    void *MBEDTLS_PRIVATE(sig_opts);             /**< Signature options to be passed to mbedtls_pk_verify_ext(), e.g. for RSASSA-PSS */

    /** Next element in the linked list of CRL.
     * \p NULL indicates the end of the list.
     * Do not modify this field directly. */
    struct mbedtls_x509_crl *next;
}
mbedtls_x509_crl;

/**
 * \brief          Parse a DER-encoded CRL and append it to the chained list
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param chain    points to the start of the chain
 * \param buf      buffer holding the CRL data in DER format
 * \param buflen   size of the buffer
 *                 (including the terminating null byte for PEM data)
 *
 * \return         0 if successful, or a specific X509 or PEM error code
 */
int mbedtls_x509_crl_parse_der(mbedtls_x509_crl *chain,
                               const unsigned char *buf, size_t buflen);
/**
 * \brief          Parse one or more CRLs and append them to the chained list
 *
 * \note           Multiple CRLs are accepted only if using PEM format
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param chain    points to the start of the chain
 * \param buf      buffer holding the CRL data in PEM or DER format
 * \param buflen   size of the buffer
 *                 (including the terminating null byte for PEM data)
 *
 * \return         0 if successful, or a specific X509 or PEM error code
 */
int mbedtls_x509_crl_parse(mbedtls_x509_crl *chain, const unsigned char *buf, size_t buflen);

#if !defined(MBEDTLS_X509_REMOVE_INFO)
/**
 * \brief          Returns an informational string about the CRL.
 *
 * \param buf      Buffer to write to
 * \param size     Maximum size of buffer
 * \param prefix   A line prefix
 * \param crl      The X509 CRL to represent
 *
 * \return         The length of the string written (not including the
 *                 terminated nul byte), or a negative error code.
 */
int mbedtls_x509_crl_info(char *buf, size_t size, const char *prefix,
                          const mbedtls_x509_crl *crl);
#endif /* !MBEDTLS_X509_REMOVE_INFO */

/**
 * \brief          Initialize a CRL (chain)
 *
 * \param crl      CRL chain to initialize
 */
void mbedtls_x509_crl_init(mbedtls_x509_crl *crl);

/**
 * \brief          Unallocate all CRL data
 *
 * \param crl      CRL chain to free
 */
void mbedtls_x509_crl_free(mbedtls_x509_crl *crl);

/** \} name Structures and functions for parsing CRLs */
/** \} addtogroup x509_module */

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_x509_crl.h */


/**
 * \addtogroup x509_module
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name Structures and functions for parsing and writing X.509 certificates
 * \{
 */

/**
 * Container for an X.509 certificate. The certificate may be chained.
 *
 * Some fields of this structure are publicly readable. Do not modify
 * them except via Mbed TLS library functions: the effect of modifying
 * those fields or the data that those fields points to is unspecified.
 */
typedef struct mbedtls_x509_crt {
    int MBEDTLS_PRIVATE(own_buffer);                     /**< Indicates if \c raw is owned
                                                          *   by the structure or not.        */
    mbedtls_x509_buf raw;               /**< The raw certificate data (DER). */
    mbedtls_x509_buf tbs;               /**< The raw certificate body (DER). The part that is To Be Signed. */

    int version;                /**< The X.509 version. (1=v1, 2=v2, 3=v3) */
    mbedtls_x509_buf serial;            /**< Unique id for certificate issued by a specific CA. */
    mbedtls_x509_buf sig_oid;           /**< Signature algorithm, e.g. sha1RSA */

    mbedtls_x509_buf issuer_raw;        /**< The raw issuer data (DER). Used for quick comparison. */
    mbedtls_x509_buf subject_raw;       /**< The raw subject data (DER). Used for quick comparison. */

    mbedtls_x509_name issuer;           /**< The parsed issuer data (named information object). */
    mbedtls_x509_name subject;          /**< The parsed subject data (named information object). */

    mbedtls_x509_time valid_from;       /**< Start time of certificate validity. */
    mbedtls_x509_time valid_to;         /**< End time of certificate validity. */

    mbedtls_x509_buf pk_raw;
    mbedtls_pk_context pk;              /**< Container for the public key context. */

    mbedtls_x509_buf issuer_id;         /**< Optional X.509 v2/v3 issuer unique identifier. */
    mbedtls_x509_buf subject_id;        /**< Optional X.509 v2/v3 subject unique identifier. */
    mbedtls_x509_buf v3_ext;            /**< Optional X.509 v3 extensions.  */
    mbedtls_x509_sequence subject_alt_names; /**< Optional list of raw entries of Subject Alternative Names extension. These can be later parsed by mbedtls_x509_parse_subject_alt_name. */
    mbedtls_x509_buf subject_key_id;    /**< Optional X.509 v3 extension subject key identifier. */
    mbedtls_x509_authority authority_key_id;    /**< Optional X.509 v3 extension authority key identifier. */

    mbedtls_x509_sequence certificate_policies; /**< Optional list of certificate policies (Only anyPolicy is printed and enforced, however the rest of the policies are still listed). */

    int MBEDTLS_PRIVATE(ext_types);              /**< Bit string containing detected and parsed extensions */
    int MBEDTLS_PRIVATE(ca_istrue);              /**< Optional Basic Constraint extension value: 1 if this certificate belongs to a CA, 0 otherwise. */
    int MBEDTLS_PRIVATE(max_pathlen);            /**< Optional Basic Constraint extension value: The maximum path length to the root certificate. Path length is 1 higher than RFC 5280 'meaning', so 1+ */

    unsigned int MBEDTLS_PRIVATE(key_usage);     /**< Optional key usage extension value: See the values in x509.h */

    mbedtls_x509_sequence ext_key_usage; /**< Optional list of extended key usage OIDs. */

    unsigned char MBEDTLS_PRIVATE(ns_cert_type); /**< Optional Netscape certificate type extension value: See the values in x509.h */

    mbedtls_x509_buf MBEDTLS_PRIVATE(sig);               /**< Signature: hash of the tbs part signed with the private key. */
    mbedtls_md_type_t MBEDTLS_PRIVATE(sig_md);           /**< Internal representation of the MD algorithm of the signature algorithm, e.g. MBEDTLS_MD_SHA256 */
    mbedtls_pk_type_t MBEDTLS_PRIVATE(sig_pk);           /**< Internal representation of the Public Key algorithm of the signature algorithm, e.g. MBEDTLS_PK_RSA */
    void *MBEDTLS_PRIVATE(sig_opts);             /**< Signature options to be passed to mbedtls_pk_verify_ext(), e.g. for RSASSA-PSS */

    /** Next certificate in the linked list that constitutes the CA chain.
     * \p NULL indicates the end of the list.
     * Do not modify this field directly. */
    struct mbedtls_x509_crt *next;
}
mbedtls_x509_crt;

/**
 * Build flag from an algorithm/curve identifier (pk, md, ecp)
 * Since 0 is always XXX_NONE, ignore it.
 */
#define MBEDTLS_X509_ID_FLAG(id)   (1 << ((id) - 1))

/**
 * Security profile for certificate verification.
 *
 * All lists are bitfields, built by ORing flags from MBEDTLS_X509_ID_FLAG().
 *
 * The fields of this structure are part of the public API and can be
 * manipulated directly by applications. Future versions of the library may
 * add extra fields or reorder existing fields.
 *
 * You can create custom profiles by starting from a copy of
 * an existing profile, such as mbedtls_x509_crt_profile_default or
 * mbedtls_x509_ctr_profile_none and then tune it to your needs.
 *
 * For example to allow SHA-224 in addition to the default:
 *
 *  mbedtls_x509_crt_profile my_profile = mbedtls_x509_crt_profile_default;
 *  my_profile.allowed_mds |= MBEDTLS_X509_ID_FLAG( MBEDTLS_MD_SHA224 );
 *
 * Or to allow only RSA-3072+ with SHA-256:
 *
 *  mbedtls_x509_crt_profile my_profile = mbedtls_x509_crt_profile_none;
 *  my_profile.allowed_mds = MBEDTLS_X509_ID_FLAG( MBEDTLS_MD_SHA256 );
 *  my_profile.allowed_pks = MBEDTLS_X509_ID_FLAG( MBEDTLS_PK_RSA );
 *  my_profile.rsa_min_bitlen = 3072;
 */
typedef struct mbedtls_x509_crt_profile {
    uint32_t allowed_mds;       /**< MDs for signatures         */
    uint32_t allowed_pks;       /**< PK algs for public keys;
                                 *   this applies to all certificates
                                 *   in the provided chain.     */
    uint32_t allowed_curves;    /**< Elliptic curves for ECDSA  */
    uint32_t rsa_min_bitlen;    /**< Minimum size for RSA keys  */
}
mbedtls_x509_crt_profile;

#define MBEDTLS_X509_CRT_VERSION_1              0
#define MBEDTLS_X509_CRT_VERSION_2              1
#define MBEDTLS_X509_CRT_VERSION_3              2

#define MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN 20
#define MBEDTLS_X509_RFC5280_UTC_TIME_LEN   15

#if !defined(MBEDTLS_X509_MAX_FILE_PATH_LEN)
#define MBEDTLS_X509_MAX_FILE_PATH_LEN 512
#endif

/* This macro unfolds to the concatenation of macro invocations
 * X509_CRT_ERROR_INFO( error code,
 *                             error code as string,
 *                             human readable description )
 * where X509_CRT_ERROR_INFO is defined by the user.
 * See x509_crt.c for an example of how to use this. */
#define MBEDTLS_X509_CRT_ERROR_INFO_LIST                                  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_EXPIRED,            \
                        "MBEDTLS_X509_BADCERT_EXPIRED",          \
                        "The certificate validity has expired") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_REVOKED,            \
                        "MBEDTLS_X509_BADCERT_REVOKED",          \
                        "The certificate has been revoked (is on a CRL)") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_CN_MISMATCH,                  \
                        "MBEDTLS_X509_BADCERT_CN_MISMATCH",                \
                        "The certificate Common Name (CN) does not match with the expected CN") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_NOT_TRUSTED,                             \
                        "MBEDTLS_X509_BADCERT_NOT_TRUSTED",                           \
                        "The certificate is not correctly signed by the trusted CA") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_NOT_TRUSTED,                      \
                        "MBEDTLS_X509_BADCRL_NOT_TRUSTED",                    \
                        "The CRL is not correctly signed by the trusted CA") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_EXPIRED,    \
                        "MBEDTLS_X509_BADCRL_EXPIRED",  \
                        "The CRL is expired")          \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_MISSING,   \
                        "MBEDTLS_X509_BADCERT_MISSING", \
                        "Certificate was missing")     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_SKIP_VERIFY,         \
                        "MBEDTLS_X509_BADCERT_SKIP_VERIFY",       \
                        "Certificate verification was skipped")  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_OTHER,                          \
                        "MBEDTLS_X509_BADCERT_OTHER",                        \
                        "Other reason (can be used by verify callback)")    \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_FUTURE,                         \
                        "MBEDTLS_X509_BADCERT_FUTURE",                       \
                        "The certificate validity starts in the future")    \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_FUTURE,     \
                        "MBEDTLS_X509_BADCRL_FUTURE",   \
                        "The CRL is from the future")  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_KEY_USAGE,                      \
                        "MBEDTLS_X509_BADCERT_KEY_USAGE",                    \
                        "Usage does not match the keyUsage extension")      \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_EXT_KEY_USAGE,                       \
                        "MBEDTLS_X509_BADCERT_EXT_KEY_USAGE",                     \
                        "Usage does not match the extendedKeyUsage extension")   \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_NS_CERT_TYPE,                        \
                        "MBEDTLS_X509_BADCERT_NS_CERT_TYPE",                      \
                        "Usage does not match the nsCertType extension")         \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_MD,                              \
                        "MBEDTLS_X509_BADCERT_BAD_MD",                            \
                        "The certificate is signed with an unacceptable hash.")  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_PK,                                                  \
                        "MBEDTLS_X509_BADCERT_BAD_PK",                                                \
                        "The certificate is signed with an unacceptable PK alg (eg RSA vs ECDSA).")  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_KEY,                                                            \
                        "MBEDTLS_X509_BADCERT_BAD_KEY",                                                          \
                        "The certificate is signed with an unacceptable key (eg bad curve, RSA too short).")    \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_MD,                          \
                        "MBEDTLS_X509_BADCRL_BAD_MD",                        \
                        "The CRL is signed with an unacceptable hash.")     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_PK,                                            \
                        "MBEDTLS_X509_BADCRL_BAD_PK",                                          \
                        "The CRL is signed with an unacceptable PK alg (eg RSA vs ECDSA).")   \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_KEY,                                                    \
                        "MBEDTLS_X509_BADCRL_BAD_KEY",                                                  \
                        "The CRL is signed with an unacceptable key (eg bad curve, RSA too short).")

/**
 * Container for writing a certificate (CRT)
 */
typedef struct mbedtls_x509write_cert {
    int MBEDTLS_PRIVATE(version);
    unsigned char MBEDTLS_PRIVATE(serial)[MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN];
    size_t MBEDTLS_PRIVATE(serial_len);
    mbedtls_pk_context *MBEDTLS_PRIVATE(subject_key);
    mbedtls_pk_context *MBEDTLS_PRIVATE(issuer_key);
    mbedtls_asn1_named_data *MBEDTLS_PRIVATE(subject);
    mbedtls_asn1_named_data *MBEDTLS_PRIVATE(issuer);
    mbedtls_md_type_t MBEDTLS_PRIVATE(md_alg);
    char MBEDTLS_PRIVATE(not_before)[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
    char MBEDTLS_PRIVATE(not_after)[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
    mbedtls_asn1_named_data *MBEDTLS_PRIVATE(extensions);
}
mbedtls_x509write_cert;

/**
 * \brief           Set Subject Alternative Name
 *
 * \param ctx       Certificate context to use
 * \param san_list  List of SAN values
 *
 * \return          0 if successful, or MBEDTLS_ERR_X509_ALLOC_FAILED
 *
 * \note            "dnsName", "uniformResourceIdentifier", "IP address",
 *                  "otherName", and "DirectoryName", as defined in RFC 5280,
 *                  are supported.
 */
int mbedtls_x509write_crt_set_subject_alternative_name(mbedtls_x509write_cert *ctx,
                                                       const mbedtls_x509_san_list *san_list);

/**
 * Item in a verification chain: cert and flags for it
 */
typedef struct {
    mbedtls_x509_crt *MBEDTLS_PRIVATE(crt);
    uint32_t MBEDTLS_PRIVATE(flags);
} mbedtls_x509_crt_verify_chain_item;

/**
 * Max size of verification chain: end-entity + intermediates + trusted root
 */
#define MBEDTLS_X509_MAX_VERIFY_CHAIN_SIZE  (MBEDTLS_X509_MAX_INTERMEDIATE_CA + 2)

/**
 * Verification chain as built by \c mbedtls_crt_verify_chain()
 */
typedef struct {
    mbedtls_x509_crt_verify_chain_item MBEDTLS_PRIVATE(items)[MBEDTLS_X509_MAX_VERIFY_CHAIN_SIZE];
    unsigned MBEDTLS_PRIVATE(len);

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
    /* This stores the list of potential trusted signers obtained from
     * the CA callback used for the CRT verification, if configured.
     * We must track it somewhere because the callback passes its
     * ownership to the caller. */
    mbedtls_x509_crt *MBEDTLS_PRIVATE(trust_ca_cb_result);
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */
} mbedtls_x509_crt_verify_chain;

/* Now we can declare functions that take a pointer to that */
typedef void mbedtls_x509_crt_restart_ctx;

/**
 * Default security profile. Should provide a good balance between security
 * and compatibility with current deployments.
 *
 * This profile permits:
 * - SHA2 hashes with at least 256 bits: SHA-256, SHA-384, SHA-512.
 * - Elliptic curves with 255 bits and above except secp256k1.
 * - RSA with 2048 bits and above.
 *
 * New minor versions of Mbed TLS may extend this profile, for example if
 * new algorithms are added to the library. New minor versions of Mbed TLS will
 * not reduce this profile unless serious security concerns require it.
 */
extern const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_default;

/**
 * Expected next default profile. Recommended for new deployments.
 * Currently targets a 128-bit security level, except for allowing RSA-2048.
 * This profile may change at any time.
 */
extern const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_next;

/**
 * NSA Suite B profile.
 */
extern const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_suiteb;

/**
 * Empty profile that allows nothing. Useful as a basis for constructing
 * custom profiles.
 */
extern const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_none;

/**
 * \brief          Parse a single DER formatted certificate and add it
 *                 to the end of the provided chained list.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param chain    The pointer to the start of the CRT chain to attach to.
 *                 When parsing the first CRT in a chain, this should point
 *                 to an instance of ::mbedtls_x509_crt initialized through
 *                 mbedtls_x509_crt_init().
 * \param buf      The buffer holding the DER encoded certificate.
 * \param buflen   The size in Bytes of \p buf.
 *
 * \note           This function makes an internal copy of the CRT buffer
 *                 \p buf. In particular, \p buf may be destroyed or reused
 *                 after this call returns. To avoid duplicating the CRT
 *                 buffer (at the cost of stricter lifetime constraints),
 *                 use mbedtls_x509_crt_parse_der_nocopy() instead.
 *
 * \return         \c 0 if successful.
 * \return         A negative error code on failure.
 */
int mbedtls_x509_crt_parse_der(mbedtls_x509_crt *chain,
                               const unsigned char *buf,
                               size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          The type of certificate extension callbacks.
 *
 *                 Callbacks of this type are passed to and used by the
 *                 mbedtls_x509_crt_parse_der_with_ext_cb() routine when
 *                 it encounters either an unsupported extension or a
 *                 "certificate policies" extension containing any
 *                 unsupported certificate policies.
 *                 Future versions of the library may invoke the callback
 *                 in other cases, if and when the need arises.
 *
 * \param p_ctx    An opaque context passed to the callback.
 * \param crt      The certificate being parsed.
 * \param oid      The OID of the extension.
 * \param critical Whether the extension is critical.
 * \param p        Pointer to the start of the extension value
 *                 (the content of the OCTET STRING).
 * \param end      End of extension value.
 *
 * \note           The callback must fail and return a negative error code
 *                 if it can not parse or does not support the extension.
 *                 When the callback fails to parse a critical extension
 *                 mbedtls_x509_crt_parse_der_with_ext_cb() also fails.
 *                 When the callback fails to parse a non critical extension
 *                 mbedtls_x509_crt_parse_der_with_ext_cb() simply skips
 *                 the extension and continues parsing.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
typedef int (*mbedtls_x509_crt_ext_cb_t)(void *p_ctx,
                                         mbedtls_x509_crt const *crt,
                                         mbedtls_x509_buf const *oid,
                                         int critical,
                                         const unsigned char *p,
                                         const unsigned char *end, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Parse one DER-encoded or one or more concatenated PEM-encoded
 *                 certificates and add them to the chained list.
 *
 *                 For CRTs in PEM encoding, the function parses permissively:
 *                 if at least one certificate can be parsed, the function
 *                 returns the number of certificates for which parsing failed
 *                 (hence \c 0 if all certificates were parsed successfully).
 *                 If no certificate could be parsed, the function returns
 *                 the first (negative) error encountered during parsing.
 *
 *                 PEM encoded certificates may be interleaved by other data
 *                 such as human readable descriptions of their content, as
 *                 long as the certificates are enclosed in the PEM specific
 *                 '-----{BEGIN/END} CERTIFICATE-----' delimiters.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param chain    The chain to which to add the parsed certificates.
 * \param buf      The buffer holding the certificate data in PEM or DER format.
 *                 For certificates in PEM encoding, this may be a concatenation
 *                 of multiple certificates; for DER encoding, the buffer must
 *                 comprise exactly one certificate.
 * \param buflen   The size of \p buf, including the terminating \c NULL byte
 *                 in case of PEM encoded data.
 *
 * \return         \c 0 if all certificates were parsed successfully.
 * \return         The (positive) number of certificates that couldn't
 *                 be parsed if parsing was partly successful (see above).
 * \return         A negative X509 or PEM error code otherwise.
 *
 */
int mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_X509_REMOVE_INFO)

#endif

/**
 * \brief          Restartable version of \c mbedtls_crt_verify_with_profile()
 *
 * \note           Performs the same job as \c mbedtls_crt_verify_with_profile()
 *                 but can return early and restart according to the limit
 *                 set with \c mbedtls_ecp_set_max_ops() to reduce blocking.
 *
 * \param crt      The certificate chain to be verified.
 * \param trust_ca The list of trusted CAs.
 * \param ca_crl   The list of CRLs for trusted CAs.
 * \param profile  The security profile to use for the verification.
 * \param cn       The expected Common Name. This may be \c NULL if the
 *                 CN need not be verified.
 * \param flags    The address at which to store the result of the verification.
 *                 If the verification couldn't be completed, the flag value is
 *                 set to (uint32_t) -1.
 * \param f_vrfy   The verification callback to use. See the documentation
 *                 of mbedtls_x509_crt_verify() for more information.
 * \param p_vrfy   The context to be passed to \p f_vrfy.
 * \param rs_ctx   The restart context to use. This may be set to \c NULL
 *                 to disable restartable ECC.
 *
 * \return         See \c mbedtls_crt_verify_with_profile(), or
 * \return         #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                 operations was reached: see \c mbedtls_ecp_set_max_ops().
 */
int mbedtls_x509_crt_verify_restartable(mbedtls_x509_crt *crt,
                                        mbedtls_x509_crt *trust_ca,
                                        mbedtls_x509_crl *ca_crl,
                                        const mbedtls_x509_crt_profile *profile,
                                        const char *cn, uint32_t *flags,
                                        int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                                        void *p_vrfy,
                                        mbedtls_x509_crt_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief               The type of trusted certificate callbacks.
 *
 *                      Callbacks of this type are passed to and used by the CRT
 *                      verification routine mbedtls_x509_crt_verify_with_ca_cb()
 *                      when looking for trusted signers of a given certificate.
 *
 *                      On success, the callback returns a list of trusted
 *                      certificates to be considered as potential signers
 *                      for the input certificate.
 *
 * \param p_ctx         An opaque context passed to the callback.
 * \param child         The certificate for which to search a potential signer.
 *                      This will point to a readable certificate.
 * \param candidate_cas The address at which to store the address of the first
 *                      entry in the generated linked list of candidate signers.
 *                      This will not be \c NULL.
 *
 * \note                The callback must only return a non-zero value on a
 *                      fatal error. If, in contrast, the search for a potential
 *                      signer completes without a single candidate, the
 *                      callback must return \c 0 and set \c *candidate_cas
 *                      to \c NULL.
 *
 * \return              \c 0 on success. In this case, \c *candidate_cas points
 *                      to a heap-allocated linked list of instances of
 *                      ::mbedtls_x509_crt, and ownership of this list is passed
 *                      to the caller.
 * \return              A negative error code on failure.
 */
typedef int (*mbedtls_x509_crt_ca_cb_t)(void *p_ctx,
                                        mbedtls_x509_crt const *child,
                                        mbedtls_x509_crt **candidate_cas, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
/**
 * \brief          Version of \c mbedtls_x509_crt_verify_with_profile() which
 *                 uses a callback to acquire the list of trusted CA
 *                 certificates.
 *
 * \param crt      The certificate chain to be verified.
 * \param f_ca_cb  The callback to be used to query for potential signers
 *                 of a given child certificate. See the documentation of
 *                 ::mbedtls_x509_crt_ca_cb_t for more information.
 * \param p_ca_cb  The opaque context to be passed to \p f_ca_cb.
 * \param profile  The security profile for the verification.
 * \param cn       The expected Common Name. This may be \c NULL if the
 *                 CN need not be verified.
 * \param flags    The address at which to store the result of the verification.
 *                 If the verification couldn't be completed, the flag value is
 *                 set to (uint32_t) -1.
 * \param f_vrfy   The verification callback to use. See the documentation
 *                 of mbedtls_x509_crt_verify() for more information.
 * \param p_vrfy   The context to be passed to \p f_vrfy.
 *
 * \return         See \c mbedtls_crt_verify_with_profile().
 */
int mbedtls_x509_crt_verify_with_ca_cb(mbedtls_x509_crt *crt,
                                       mbedtls_x509_crt_ca_cb_t f_ca_cb,
                                       void *p_ca_cb,
                                       const mbedtls_x509_crt_profile *profile,
                                       const char *cn, uint32_t *flags,
                                       int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                                       void *p_vrfy, MinTlsDiagnostics* diagnostics);

#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */

/**
 * \brief          Check usage of certificate against keyUsage extension.
 *
 * \param crt      Leaf certificate used.
 * \param usage    Intended usage(s) (eg MBEDTLS_X509_KU_KEY_ENCIPHERMENT
 *                 before using the certificate to perform an RSA key
 *                 exchange).
 *
 * \note           Except for decipherOnly and encipherOnly, a bit set in the
 *                 usage argument means this bit MUST be set in the
 *                 certificate. For decipherOnly and encipherOnly, it means
 *                 that bit MAY be set.
 *
 * \return         0 is these uses of the certificate are allowed,
 *                 MBEDTLS_ERR_X509_BAD_INPUT_DATA if the keyUsage extension
 *                 is present but does not match the usage argument.
 *
 * \note           You should only call this function on leaf certificates, on
 *                 (intermediate) CAs the keyUsage extension is automatically
 *                 checked by \c mbedtls_x509_crt_verify().
 */
int mbedtls_x509_crt_check_key_usage(const mbedtls_x509_crt *crt,
                                     unsigned int usage, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Check usage of certificate against extendedKeyUsage.
 *
 * \param crt       Leaf certificate used.
 * \param usage_oid Intended usage (eg MBEDTLS_OID_SERVER_AUTH or
 *                  MBEDTLS_OID_CLIENT_AUTH).
 * \param usage_len Length of usage_oid (eg given by MBEDTLS_OID_SIZE()).
 *
 * \return          0 if this use of the certificate is allowed,
 *                  MBEDTLS_ERR_X509_BAD_INPUT_DATA if not.
 *
 * \note            Usually only makes sense on leaf certificates.
 */
int mbedtls_x509_crt_check_extended_key_usage(const mbedtls_x509_crt *crt,
                                              const char *usage_oid,
                                              size_t usage_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Initialize a certificate (chain)
 *
 * \param crt      Certificate chain to initialize
 */
void mbedtls_x509_crt_init(mbedtls_x509_crt *crt);

/**
 * \brief          Unallocate all certificate data
 *
 * \param crt      Certificate chain to free
 */
void mbedtls_x509_crt_free(mbedtls_x509_crt *crt);

/** \} name Structures and functions for parsing and writing X.509 certificates */

/** \} addtogroup x509_module */

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_x509_crt.h */



#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED)
#endif


/*
 * SSL Error codes
 */
/** A cryptographic operation is in progress. Try again later. */
#define MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS                -0x7000
/** The requested feature is not available. */
#define MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE               -0x7080
/** Bad input parameters to function. */
#define MBEDTLS_ERR_SSL_BAD_INPUT_DATA                    -0x7100
/** Verification of the message MAC failed. */
#define MBEDTLS_ERR_SSL_INVALID_MAC                       -0x7180
/** An invalid SSL record was received. */
#define MBEDTLS_ERR_SSL_INVALID_RECORD                    -0x7200
/** The connection indicated an EOF. */
#define MBEDTLS_ERR_SSL_CONN_EOF                          -0x7280
/** A message could not be parsed due to a syntactic error. */
#define MBEDTLS_ERR_SSL_DECODE_ERROR                      -0x7300
/* Error space gap */
/** No RNG was provided to the SSL module. */
#define MBEDTLS_ERR_SSL_NO_RNG                            -0x7400
/** No client certification received from the client, but required by the authentication mode. */
#define MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE             -0x7480
/** Client received an extended server hello containing an unsupported extension */
#define MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION             -0x7500
/** No ALPN protocols supported that the client advertises */
#define MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL           -0x7580
/** The own private key or pre-shared key is not set, but needed. */
#define MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED              -0x7600
/** No CA Chain is set, but required to operate. */
#define MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED                 -0x7680
/** An unexpected message was received from our peer. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE                -0x7700
/** A fatal alert message was received from our peer. */
#define MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE               -0x7780
/** No server could be identified matching the client's SNI. */
#define MBEDTLS_ERR_SSL_UNRECOGNIZED_NAME                 -0x7800
/** The peer notified us that the connection is going to be closed. */
#define MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY                 -0x7880
/* Error space gap */
/* Error space gap */
/** Processing of the Certificate handshake message failed. */
#define MBEDTLS_ERR_SSL_BAD_CERTIFICATE                   -0x7A00
/* Error space gap */
/** A TLS 1.3 NewSessionTicket message has been received. */
#define MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET       -0x7B00
/** Not possible to read early data */
#define MBEDTLS_ERR_SSL_CANNOT_READ_EARLY_DATA            -0x7B80
/**
 * Early data has been received as part of an on-going handshake.
 * This error code can be returned only on server side if and only if early
 * data has been enabled by means of the mbedtls_ssl_conf_early_data() API.
 * This error code can then be returned by mbedtls_ssl_handshake(),
 * mbedtls_ssl_handshake_step(), mbedtls_ssl_read() or mbedtls_ssl_write() if
 * early data has been received as part of the handshake sequence they
 * triggered. To read the early data, call mbedtls_ssl_read_early_data().
 */
#define MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA               -0x7C00
/** Not possible to write early data */
#define MBEDTLS_ERR_SSL_CANNOT_WRITE_EARLY_DATA           -0x7C80
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/** Cache entry not found */
#define MBEDTLS_ERR_SSL_CACHE_ENTRY_NOT_FOUND             -0x7E80
/** Memory allocation failed */
#define MBEDTLS_ERR_SSL_ALLOC_FAILED                      -0x7F00
/** Hardware acceleration function returned with error */
#define MBEDTLS_ERR_SSL_HW_ACCEL_FAILED                   -0x7F80
/** Hardware acceleration function skipped / left alone data */
#define MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH              -0x6F80
/** Handshake protocol not within min/max boundaries */
#define MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION              -0x6E80
/** The handshake negotiation failed. */
#define MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE                 -0x6E00
/** Session ticket has expired. */
#define MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED            -0x6D80
/** Public key type mismatch (eg, asked for RSA key exchange and presented EC key) */
#define MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH                  -0x6D00
/** Unknown identity received (eg, PSK identity) */
#define MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY                  -0x6C80
/** Internal error (eg, unexpected failure in lower-level module) */
#define MBEDTLS_ERR_SSL_INTERNAL_ERROR                    -0x6C00
/** A counter would wrap (eg, too many messages exchanged). */
#define MBEDTLS_ERR_SSL_COUNTER_WRAPPING                  -0x6B80
/** Unexpected message at ServerHello in renegotiation. */
#define MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO       -0x6B00
/** DTLS client must retry for hello verification */
#define MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED             -0x6A80
/** A buffer is too small to receive or write a message */
#define MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL                  -0x6A00
/* Error space gap */
/** No data of requested type currently available on underlying transport. */
#define MBEDTLS_ERR_SSL_WANT_READ                         -0x6900
/** Connection requires a write call. */
#define MBEDTLS_ERR_SSL_WANT_WRITE                        -0x6880
/** The operation timed out. */
#define MBEDTLS_ERR_SSL_TIMEOUT                           -0x6800
/** The client initiated a reconnect from the same port. */
#define MBEDTLS_ERR_SSL_CLIENT_RECONNECT                  -0x6780
/** Record header looks valid but is not expected. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_RECORD                 -0x6700
/** The alert message received indicates a non-fatal error. */
#define MBEDTLS_ERR_SSL_NON_FATAL                         -0x6680
/** A field in a message was incorrect or inconsistent with other fields. */
#define MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER                 -0x6600
/** Internal-only message signaling that further message-processing should be done */
#define MBEDTLS_ERR_SSL_CONTINUE_PROCESSING               -0x6580
/** The asynchronous operation is not completed yet. */
#define MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS                 -0x6500
/** Internal-only message signaling that a message arrived early. */
#define MBEDTLS_ERR_SSL_EARLY_MESSAGE                     -0x6480
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/** An encrypted DTLS-frame with an unexpected CID was received. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_CID                    -0x6000
/** An operation failed due to an unexpected version or configuration. */
#define MBEDTLS_ERR_SSL_VERSION_MISMATCH                  -0x5F00
/** Invalid value in SSL config */
#define MBEDTLS_ERR_SSL_BAD_CONFIG                        -0x5E80
/* Error space gap */
/** Attempt to verify a certificate without an expected hostname.
 * This is usually insecure.
 *
 * In TLS clients, when a client authenticates a server through its
 * certificate, the client normally checks three things:
 * - the certificate chain must be valid;
 * - the chain must start from a trusted CA;
 * - the certificate must cover the server name that is expected by the client.
 *
 * Omitting any of these checks is generally insecure, and can allow a
 * malicious server to impersonate a legitimate server.
 *
 * The third check may be safely skipped in some unusual scenarios,
 * such as networks where eavesdropping is a risk but not active attacks,
 * or a private PKI where the client equally trusts all servers that are
 * accredited by the root CA.
 *
 * You should call mbedtls_ssl_set_hostname() with the expected server name
 * before starting a TLS handshake on a client (unless the client is
 * set up to only use PSK-based authentication, which does not rely on the
 * host name). If you have determined that server name verification is not
 * required for security in your scenario, call mbedtls_ssl_set_hostname()
 * with \p NULL as the server name.
 *
 * This error is raised if all of the following conditions are met:
 *
 * - A TLS client is configured with the authentication mode
 *   #MBEDTLS_SSL_VERIFY_REQUIRED (default).
 * - Certificate authentication is enabled.
 * - The client does not call mbedtls_ssl_set_hostname().
 * - The configuration option
 *   #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *   is not enabled.
 */
#define MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME  -0x5D80

/*
 * Constants from RFC 8446 for TLS 1.3 PSK modes
 *
 * Those are used in the Pre-Shared Key Exchange Modes extension.
 * See Section 4.2.9 in RFC 8446.
 */
#define MBEDTLS_SSL_TLS1_3_PSK_MODE_PURE  0 /* Pure PSK-based exchange  */
#define MBEDTLS_SSL_TLS1_3_PSK_MODE_ECDHE 1 /* PSK+ECDHE-based exchange */

/*
 * TLS 1.3 NamedGroup values
 *
 * From RF 8446
 *    enum {
 *         // Elliptic Curve Groups (ECDHE)
 *         secp256r1(0x0017), secp384r1(0x0018), secp521r1(0x0019),
 *         x25519(0x001D), x448(0x001E),
 *         // Finite Field Groups (DHE)
 *         ffdhe2048(0x0100), ffdhe3072(0x0101), ffdhe4096(0x0102),
 *         ffdhe6144(0x0103), ffdhe8192(0x0104),
 *         // Reserved Code Points
 *         ffdhe_private_use(0x01FC..0x01FF),
 *         ecdhe_private_use(0xFE00..0xFEFF),
 *         (0xFFFF)
 *     } NamedGroup;
 *
 */

/* Elliptic Curve Groups (ECDHE) */
#define MBEDTLS_SSL_IANA_TLS_GROUP_NONE               0
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP192K1     0x0012
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP192R1     0x0013
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP224K1     0x0014
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP224R1     0x0015
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP256K1     0x0016
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1     0x0017
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1     0x0018
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1     0x0019
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP256R1       0x001A
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP384R1       0x001B
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP512R1       0x001C
#define MBEDTLS_SSL_IANA_TLS_GROUP_X25519        0x001D
#define MBEDTLS_SSL_IANA_TLS_GROUP_X448          0x001E
/* Finite Field Groups (DHE) */
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE2048     0x0100
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE3072     0x0101
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE4096     0x0102
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE6144     0x0103
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE8192     0x0104

/*
 * TLS 1.3 Key Exchange Modes
 *
 * Mbed TLS internal identifiers for use with the SSL configuration API
 * mbedtls_ssl_conf_tls13_key_exchange_modes().
 */

#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK            (1u << 0)   /*!< Pure-PSK TLS 1.3 key exchange,
                                                                         *   encompassing both externally agreed PSKs
                                                                         *   as well as resumption PSKs. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL      (1u << 1)   /*!< Pure-Ephemeral TLS 1.3 key exchanges,
                                                                         *   including for example ECDHE and DHE
                                                                         *   key exchanges. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL  (1u << 2)   /*!< PSK-Ephemeral TLS 1.3 key exchanges,
                                                                         *   using both a PSK and an ephemeral
                                                                         *   key exchange. */

/* Convenience macros for sets of key exchanges. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_ALL                         \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK              |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL    |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL)          /*!< All TLS 1.3 key exchanges           */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ALL                     \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK              |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL)      /*!< All PSK-based TLS 1.3 key exchanges */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ALL               \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL        |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL)      /*!< All ephemeral TLS 1.3 key exchanges */

#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_NONE   (0)

/*
 * Various constants
 */

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/* These are the high and low bytes of ProtocolVersion as defined by:
 * - RFC 5246: ProtocolVersion version = { 3, 3 };     // TLS v1.2
 * - RFC 8446: see section 4.2.1
 */
#define MBEDTLS_SSL_MAJOR_VERSION_3             3
#define MBEDTLS_SSL_MINOR_VERSION_3             3   /*!< TLS v1.2 */
#define MBEDTLS_SSL_MINOR_VERSION_4             4   /*!< TLS v1.3 */
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#define MBEDTLS_SSL_TRANSPORT_STREAM            0   /*!< TLS      */
#define MBEDTLS_SSL_TRANSPORT_DATAGRAM          1   /*!< DTLS     */

#define MBEDTLS_SSL_MAX_HOST_NAME_LEN           255 /*!< Maximum host name defined in RFC 1035 */
#define MBEDTLS_SSL_MAX_ALPN_NAME_LEN           255 /*!< Maximum size in bytes of a protocol name in alpn ext., RFC 7301 */

#define MBEDTLS_SSL_MAX_ALPN_LIST_LEN           65535 /*!< Maximum size in bytes of list in alpn ext., RFC 7301          */

/* RFC 6066 section 4, see also mfl_code_to_length in ssl_tls.c
 * NONE must be zero so that memset()ing structure to zero works */
#define MBEDTLS_SSL_MAX_FRAG_LEN_NONE           0   /*!< don't use this extension   */
#define MBEDTLS_SSL_MAX_FRAG_LEN_512            1   /*!< MaxFragmentLength 2^9      */
#define MBEDTLS_SSL_MAX_FRAG_LEN_1024           2   /*!< MaxFragmentLength 2^10     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_2048           3   /*!< MaxFragmentLength 2^11     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_4096           4   /*!< MaxFragmentLength 2^12     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_INVALID        5   /*!< first invalid value        */

#define MBEDTLS_SSL_IS_CLIENT                   0
#define MBEDTLS_SSL_IS_SERVER                   1

#define MBEDTLS_SSL_EXTENDED_MS_DISABLED        0
#define MBEDTLS_SSL_EXTENDED_MS_ENABLED         1

#define MBEDTLS_SSL_CID_DISABLED                0
#define MBEDTLS_SSL_CID_ENABLED                 1

#define MBEDTLS_SSL_ETM_DISABLED                0
#define MBEDTLS_SSL_ETM_ENABLED                 1

#define MBEDTLS_SSL_COMPRESS_NULL               0

#define MBEDTLS_SSL_VERIFY_NONE                 0
#define MBEDTLS_SSL_VERIFY_OPTIONAL             1
#define MBEDTLS_SSL_VERIFY_REQUIRED             2
#define MBEDTLS_SSL_VERIFY_UNSET                3 /* Used only for sni_authmode */

#define MBEDTLS_SSL_LEGACY_RENEGOTIATION        0
#define MBEDTLS_SSL_SECURE_RENEGOTIATION        1

#define MBEDTLS_SSL_RENEGOTIATION_DISABLED      0
#define MBEDTLS_SSL_RENEGOTIATION_ENABLED       1

#define MBEDTLS_SSL_ANTI_REPLAY_DISABLED        0
#define MBEDTLS_SSL_ANTI_REPLAY_ENABLED         1

#define MBEDTLS_SSL_RENEGOTIATION_NOT_ENFORCED  -1
#define MBEDTLS_SSL_RENEGO_MAX_RECORDS_DEFAULT  16

#define MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION     0
#define MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION  1
#define MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE      2

#define MBEDTLS_SSL_TRUNC_HMAC_DISABLED         0
#define MBEDTLS_SSL_TRUNC_HMAC_ENABLED          1
#define MBEDTLS_SSL_TRUNCATED_HMAC_LEN          10  /* 80 bits, rfc 6066 section 7 */

#define MBEDTLS_SSL_SESSION_TICKETS_DISABLED     0
#define MBEDTLS_SSL_SESSION_TICKETS_ENABLED      1

#define MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_DISABLED  0
#define MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_ENABLED   1

#define MBEDTLS_SSL_PRESET_DEFAULT              0
#define MBEDTLS_SSL_PRESET_SUITEB               2

#define MBEDTLS_SSL_CERT_REQ_CA_LIST_ENABLED       1
#define MBEDTLS_SSL_CERT_REQ_CA_LIST_DISABLED      0

#define MBEDTLS_SSL_EARLY_DATA_DISABLED        0
#define MBEDTLS_SSL_EARLY_DATA_ENABLED         1

#define MBEDTLS_SSL_DTLS_SRTP_MKI_UNSUPPORTED    0
#define MBEDTLS_SSL_DTLS_SRTP_MKI_SUPPORTED      1

#define MBEDTLS_SSL_SRV_CIPHERSUITE_ORDER_CLIENT  1
#define MBEDTLS_SSL_SRV_CIPHERSUITE_ORDER_SERVER  0

/*
 * Default range for DTLS retransmission timer value, in milliseconds.
 * RFC 6347 4.2.4.1 says from 1 second to 60 seconds.
 */
#define MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MIN    1000
#define MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MAX   60000

/*
 * Whether early data record should be discarded or not and how.
 *
 * The client has indicated early data and the server has rejected them.
 * The server has then to skip past early data by either:
 * - attempting to deprotect received records using the handshake traffic
 *   key, discarding records which fail deprotection (up to the configured
 *   max_early_data_size). Once a record is deprotected successfully,
 *   it is treated as the start of the client's second flight and the
 *   server proceeds as with an ordinary 1-RTT handshake.
 * - skipping all records with an external content type of
 *   "application_data" (indicating that they are encrypted), up to the
 *   configured max_early_data_size. This is the expected behavior if the
 *   server has sent an HelloRetryRequest message. The server ignores
 *   application data message before 2nd ClientHello.
 */
#define MBEDTLS_SSL_EARLY_DATA_NO_DISCARD 0
#define MBEDTLS_SSL_EARLY_DATA_TRY_TO_DEPROTECT_AND_DISCARD 1
#define MBEDTLS_SSL_EARLY_DATA_DISCARD 2

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h or define them on the compiler command line.
 * \{
 */

/*
 * Maximum fragment length in bytes,
 * determines the size of each of the two internal I/O buffers.
 *
 * Note: the RFC defines the default size of SSL / TLS messages. If you
 * change the value here, other clients / servers may not be able to
 * communicate with you anymore. Only change this value if you control
 * both sides of the connection and have it reduced at both sides, or
 * if you're using the Max Fragment Length extension and you know all your
 * peers are using it too!
 */

/*
 * Maximum number of heap-allocated bytes for the purpose of
 * DTLS handshake message reassembly and future message buffering.
 */
#if !defined(MBEDTLS_SSL_DTLS_MAX_BUFFERING)
#define MBEDTLS_SSL_DTLS_MAX_BUFFERING 32768
#endif

/*
 * Maximum length of CIDs for incoming and outgoing messages.
 */
#if !defined(MBEDTLS_SSL_CID_IN_LEN_MAX)
#define MBEDTLS_SSL_CID_IN_LEN_MAX          32
#endif

#if !defined(MBEDTLS_SSL_CID_OUT_LEN_MAX)
#define MBEDTLS_SSL_CID_OUT_LEN_MAX         32
#endif

#if !defined(MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY)
#define MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY 16
#endif

#if !defined(MBEDTLS_SSL_MAX_EARLY_DATA_SIZE)
#define MBEDTLS_SSL_MAX_EARLY_DATA_SIZE        1024
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_TICKET_AGE_TOLERANCE)
#define MBEDTLS_SSL_TLS1_3_TICKET_AGE_TOLERANCE 6000
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH)
#define MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH 32
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_DEFAULT_NEW_SESSION_TICKETS)
#define MBEDTLS_SSL_TLS1_3_DEFAULT_NEW_SESSION_TICKETS 1
#endif

/** \} name SECTION: Module settings */

/*
 * Default to standard CID mode
 */

/*
 * Length of the verify data for secure renegotiation
 */
#define MBEDTLS_SSL_VERIFY_DATA_MAX_LEN 12

/*
 * Signaling ciphersuite values (SCSV)
 */
#define MBEDTLS_SSL_EMPTY_RENEGOTIATION_INFO    0xFF   /**< renegotiation info ext */

/*
 * Supported Signature and Hash algorithms (For TLS 1.2)
 * RFC 5246 section 7.4.1.4.1
 */
#define MBEDTLS_SSL_HASH_NONE                0
#define MBEDTLS_SSL_HASH_MD5                 1
#define MBEDTLS_SSL_HASH_SHA1                2
#define MBEDTLS_SSL_HASH_SHA224              3
#define MBEDTLS_SSL_HASH_SHA256              4
#define MBEDTLS_SSL_HASH_SHA384              5
#define MBEDTLS_SSL_HASH_SHA512              6

#define MBEDTLS_SSL_SIG_ANON                 0
#define MBEDTLS_SSL_SIG_RSA                  1
#define MBEDTLS_SSL_SIG_ECDSA                3

/*
 * TLS 1.3 signature algorithms
 * RFC 8446, Section 4.2.3
 */

/* RSASSA-PKCS1-v1_5 algorithms */
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA256 0x0401
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA384 0x0501
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA512 0x0601

/* ECDSA algorithms */
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256 0x0403
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384 0x0503
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP521R1_SHA512 0x0603

/* RSASSA-PSS algorithms with public key OID rsaEncryption */
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256 0x0804
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384 0x0805
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512 0x0806

/* EdDSA algorithms */
#define MBEDTLS_TLS1_3_SIG_ED25519 0x0807
#define MBEDTLS_TLS1_3_SIG_ED448 0x0808

/* RSASSA-PSS algorithms with public key OID RSASSA-PSS  */
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA256 0x0809
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA384 0x080A
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA512 0x080B

/* LEGACY ALGORITHMS */
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA1 0x0201
#define MBEDTLS_TLS1_3_SIG_ECDSA_SHA1     0x0203

#define MBEDTLS_TLS1_3_SIG_NONE 0x0

/*
 * Client Certificate Types
 * RFC 5246 section 7.4.4 plus RFC 4492 section 5.5
 */
#define MBEDTLS_SSL_CERT_TYPE_RSA_SIGN       1
#define MBEDTLS_SSL_CERT_TYPE_ECDSA_SIGN    64

/*
 * Message, alert and handshake types
 */
#define MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC     20
#define MBEDTLS_SSL_MSG_ALERT                  21
#define MBEDTLS_SSL_MSG_HANDSHAKE              22
#define MBEDTLS_SSL_MSG_APPLICATION_DATA       23
#define MBEDTLS_SSL_MSG_CID                    25

#define MBEDTLS_SSL_ALERT_LEVEL_WARNING         1
#define MBEDTLS_SSL_ALERT_LEVEL_FATAL           2

#define MBEDTLS_SSL_ALERT_MSG_CLOSE_NOTIFY           0  /* 0x00 */
#define MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE    10  /* 0x0A */
#define MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC        20  /* 0x14 */
#define MBEDTLS_SSL_ALERT_MSG_DECRYPTION_FAILED     21  /* 0x15 */
#define MBEDTLS_SSL_ALERT_MSG_RECORD_OVERFLOW       22  /* 0x16 */
#define MBEDTLS_SSL_ALERT_MSG_DECOMPRESSION_FAILURE 30  /* 0x1E */
#define MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE     40  /* 0x28 */
#define MBEDTLS_SSL_ALERT_MSG_NO_CERT               41  /* 0x29 */
#define MBEDTLS_SSL_ALERT_MSG_BAD_CERT              42  /* 0x2A */
#define MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT      43  /* 0x2B */
#define MBEDTLS_SSL_ALERT_MSG_CERT_REVOKED          44  /* 0x2C */
#define MBEDTLS_SSL_ALERT_MSG_CERT_EXPIRED          45  /* 0x2D */
#define MBEDTLS_SSL_ALERT_MSG_CERT_UNKNOWN          46  /* 0x2E */
#define MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER     47  /* 0x2F */
#define MBEDTLS_SSL_ALERT_MSG_UNKNOWN_CA            48  /* 0x30 */
#define MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED         49  /* 0x31 */
#define MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR          50  /* 0x32 */
#define MBEDTLS_SSL_ALERT_MSG_DECRYPT_ERROR         51  /* 0x33 */
#define MBEDTLS_SSL_ALERT_MSG_EXPORT_RESTRICTION    60  /* 0x3C */
#define MBEDTLS_SSL_ALERT_MSG_PROTOCOL_VERSION      70  /* 0x46 */
#define MBEDTLS_SSL_ALERT_MSG_INSUFFICIENT_SECURITY 71  /* 0x47 */
#define MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR        80  /* 0x50 */
#define MBEDTLS_SSL_ALERT_MSG_INAPROPRIATE_FALLBACK 86  /* 0x56 */
#define MBEDTLS_SSL_ALERT_MSG_USER_CANCELED         90  /* 0x5A */
#define MBEDTLS_SSL_ALERT_MSG_NO_RENEGOTIATION     100  /* 0x64 */
#define MBEDTLS_SSL_ALERT_MSG_MISSING_EXTENSION    109  /* 0x6d -- new in TLS 1.3 */
#define MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT      110  /* 0x6E */
#define MBEDTLS_SSL_ALERT_MSG_UNRECOGNIZED_NAME    112  /* 0x70 */
#define MBEDTLS_SSL_ALERT_MSG_UNKNOWN_PSK_IDENTITY 115  /* 0x73 */
#define MBEDTLS_SSL_ALERT_MSG_CERT_REQUIRED        116  /* 0x74 */
#define MBEDTLS_SSL_ALERT_MSG_NO_APPLICATION_PROTOCOL 120 /* 0x78 */

#define MBEDTLS_SSL_HS_HELLO_REQUEST            0
#define MBEDTLS_SSL_HS_CLIENT_HELLO             1
#define MBEDTLS_SSL_HS_SERVER_HELLO             2
#define MBEDTLS_SSL_HS_HELLO_VERIFY_REQUEST     3
#define MBEDTLS_SSL_HS_NEW_SESSION_TICKET       4
#define MBEDTLS_SSL_HS_END_OF_EARLY_DATA        5
#define MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS     8
#define MBEDTLS_SSL_HS_CERTIFICATE             11
#define MBEDTLS_SSL_HS_SERVER_KEY_EXCHANGE     12
#define MBEDTLS_SSL_HS_CERTIFICATE_REQUEST     13
#define MBEDTLS_SSL_HS_SERVER_HELLO_DONE       14
#define MBEDTLS_SSL_HS_CERTIFICATE_VERIFY      15
#define MBEDTLS_SSL_HS_CLIENT_KEY_EXCHANGE     16
#define MBEDTLS_SSL_HS_FINISHED                20
#define MBEDTLS_SSL_HS_MESSAGE_HASH           254

/*
 * TLS extensions
 */
#define MBEDTLS_TLS_EXT_SERVERNAME                   0
#define MBEDTLS_TLS_EXT_SERVERNAME_HOSTNAME          0

#define MBEDTLS_TLS_EXT_MAX_FRAGMENT_LENGTH          1

#define MBEDTLS_TLS_EXT_TRUNCATED_HMAC               4
#define MBEDTLS_TLS_EXT_STATUS_REQUEST               5 /* RFC 6066 TLS 1.2 and 1.3 */

#define MBEDTLS_TLS_EXT_SUPPORTED_ELLIPTIC_CURVES   10
#define MBEDTLS_TLS_EXT_SUPPORTED_GROUPS            10 /* RFC 8422,7919 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS     11

#define MBEDTLS_TLS_EXT_SIG_ALG                     13 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_USE_SRTP                    14
#define MBEDTLS_TLS_EXT_HEARTBEAT                   15 /* RFC 6520 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_ALPN                        16

#define MBEDTLS_TLS_EXT_SCT                         18 /* RFC 6962 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_CLI_CERT_TYPE               19 /* RFC 7250 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_SERV_CERT_TYPE              20 /* RFC 7250 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_PADDING                     21 /* RFC 7685 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_ENCRYPT_THEN_MAC            22 /* 0x16 */
#define MBEDTLS_TLS_EXT_EXTENDED_MASTER_SECRET  0x0017 /* 23 */

#define MBEDTLS_TLS_EXT_RECORD_SIZE_LIMIT           28 /* RFC 8449 (implemented for TLS 1.3 only) */

#define MBEDTLS_TLS_EXT_SESSION_TICKET              35

#define MBEDTLS_TLS_EXT_PRE_SHARED_KEY              41 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_EARLY_DATA                  42 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS          43 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_COOKIE                      44 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_PSK_KEY_EXCHANGE_MODES      45 /* RFC 8446 TLS 1.3 */

#define MBEDTLS_TLS_EXT_CERT_AUTH                   47 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_OID_FILTERS                 48 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_POST_HANDSHAKE_AUTH         49 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_SIG_ALG_CERT                50 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_KEY_SHARE                   51 /* RFC 8446 TLS 1.3 */

#if MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 0
#define MBEDTLS_TLS_EXT_CID                         54 /* RFC 9146 DTLS 1.2 CID */
#else
#define MBEDTLS_TLS_EXT_CID                        254 /* Pre-RFC 9146 DTLS 1.2 CID */
#endif

#define MBEDTLS_TLS_EXT_ECJPAKE_KKPP               256 /* experimental */

#define MBEDTLS_TLS_EXT_RENEGOTIATION_INFO      0xFF01

/*
 * Size defines
 */
#if !defined(MBEDTLS_PSK_MAX_LEN)
/*
 * If the library supports TLS 1.3 tickets and the cipher suite
 * TLS1-3-AES-256-GCM-SHA384, set the PSK maximum length to 48 instead of 32.
 * That way, the TLS 1.3 client and server are able to resume sessions where
 * the cipher suite is TLS1-3-AES-256-GCM-SHA384 (pre-shared keys are 48
 * bytes long in that case).
 */
#define MBEDTLS_PSK_MAX_LEN 32 /* 256 bits */
#endif /* !MBEDTLS_PSK_MAX_LEN */

/* Dummy type used only for its size */
union mbedtls_ssl_premaster_secret {
    unsigned char dummy; /* Make the union non-empty even with SSL disabled */
    unsigned char _pms_ecdh[MBEDTLS_ECP_MAX_BYTES];    /* RFC 4492 5.10 */
};

#define MBEDTLS_PREMASTER_SIZE     sizeof(union mbedtls_ssl_premaster_secret)

/* Length in number of bytes of the TLS sequence number */
#define MBEDTLS_SSL_SEQUENCE_NUMBER_LEN 8

/* Helper to state that client_random and server_random need to be stored
 * after the handshake is complete. This is required for context serialization
 * and for the keying material exporter in TLS 1.2. */
#if defined(MBEDTLS_SSL_CONTEXT_SERIALIZATION) || \
    (defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT) && defined(MBEDTLS_SSL_PROTO_TLS1_2))
#define MBEDTLS_SSL_KEEP_RANDBYTES
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SSL state machine
 */
typedef enum {
    MBEDTLS_SSL_HELLO_REQUEST,
    MBEDTLS_SSL_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_HELLO,
    MBEDTLS_SSL_SERVER_CERTIFICATE,
    MBEDTLS_SSL_SERVER_KEY_EXCHANGE,
    MBEDTLS_SSL_CERTIFICATE_REQUEST,
    MBEDTLS_SSL_SERVER_HELLO_DONE,
    MBEDTLS_SSL_CLIENT_CERTIFICATE,
    MBEDTLS_SSL_CLIENT_KEY_EXCHANGE,
    MBEDTLS_SSL_CERTIFICATE_VERIFY,
    MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC,
    MBEDTLS_SSL_CLIENT_FINISHED,
    MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC,
    MBEDTLS_SSL_SERVER_FINISHED,
    MBEDTLS_SSL_FLUSH_BUFFERS,
    MBEDTLS_SSL_HANDSHAKE_WRAPUP,
    MBEDTLS_SSL_NEW_SESSION_TICKET,
    MBEDTLS_SSL_SERVER_HELLO_VERIFY_REQUEST_SENT,
    MBEDTLS_SSL_HELLO_RETRY_REQUEST,
    MBEDTLS_SSL_ENCRYPTED_EXTENSIONS,
    MBEDTLS_SSL_END_OF_EARLY_DATA,
    MBEDTLS_SSL_CLIENT_CERTIFICATE_VERIFY,
    MBEDTLS_SSL_CLIENT_CCS_AFTER_SERVER_FINISHED,
    MBEDTLS_SSL_CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_CCS_AFTER_SERVER_HELLO,
    MBEDTLS_SSL_CLIENT_CCS_AFTER_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_CCS_AFTER_HELLO_RETRY_REQUEST,
    MBEDTLS_SSL_HANDSHAKE_OVER,
    MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET,
    MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET_FLUSH,
}
mbedtls_ssl_states;

/*
 * Early data status, client side only.
 */

/**
 * \brief          Callback type: send data on the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the send callback (typically a file descriptor)
 * \param buf      Buffer holding the data to send
 * \param len      Length of the data to send
 *
 * \return         The callback must return the number of bytes sent if any,
 *                 or a non-zero error code.
 *                 If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_WRITE
 *                 must be returned when the operation would block.
 *
 * \note           The callback is allowed to send fewer bytes than requested.
 *                 It must always return the number of bytes actually sent.
 */
typedef int mbedtls_ssl_send_t(void *ctx,
                               const unsigned char *buf,
                               size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Callback type: receive data from the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the receive callback (typically a file
 *                 descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 *
 * \returns        If data has been received, the positive number of bytes received.
 * \returns        \c 0 if the connection has been closed.
 * \returns        If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_READ
 *                 must be returned when the operation would block.
 * \returns        Another negative error code on other kinds of failures.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
typedef int mbedtls_ssl_recv_t(void *ctx,
                               unsigned char *buf,
                               size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Callback type: receive data from the network, with timeout
 *
 * \note           That callback must block until data is received, or the
 *                 timeout delay expires, or the operation is interrupted by a
 *                 signal.
 *
 * \param ctx      Context for the receive callback (typically a file descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 * \param timeout  Maximum number of milliseconds to wait for data
 *                 0 means no timeout (potentially waiting forever)
 *
 * \return         The callback must return the number of bytes received,
 *                 or a non-zero error code:
 *                 \c MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out,
 *                 \c MBEDTLS_ERR_SSL_WANT_READ if interrupted by a signal.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
typedef int mbedtls_ssl_recv_timeout_t(void *ctx,
                                       unsigned char *buf,
                                       size_t len,
                                       uint32_t timeout, MinTlsDiagnostics* diagnostics);
/**
 * \brief          Callback type: set a pair of timers/delays to watch
 *
 * \param ctx      Context pointer
 * \param int_ms   Intermediate delay in milliseconds
 * \param fin_ms   Final delay in milliseconds
 *                 0 cancels the current timer.
 *
 * \note           This callback must at least store the necessary information
 *                 for the associated \c mbedtls_ssl_get_timer_t callback to
 *                 return correct information.
 *
 * \note           If using an event-driven style of programming, an event must
 *                 be generated when the final delay is passed. The event must
 *                 cause a call to \c mbedtls_ssl_handshake() with the proper
 *                 SSL context to be scheduled. Care must be taken to ensure
 *                 that at most one such call happens at a time.
 *
 * \note           Only one timer at a time must be running. Calling this
 *                 function while a timer is running must cancel it. Cancelled
 *                 timers must not generate any event.
 */
typedef void mbedtls_ssl_set_timer_t(void *ctx,
                                     uint32_t int_ms,
                                     uint32_t fin_ms);

/**
 * \brief          Callback type: get status of timers/delays
 *
 * \param ctx      Context pointer
 *
 * \return         This callback must return:
 *                 -1 if cancelled (fin_ms == 0),
 *                  0 if none of the delays have passed,
 *                  1 if only the intermediate delay has passed,
 *                  2 if the final delay has passed.
 */
typedef int mbedtls_ssl_get_timer_t(void *ctx);

/* Defined below */
typedef struct mbedtls_ssl_session mbedtls_ssl_session;
typedef struct mbedtls_ssl_context mbedtls_ssl_context;
typedef struct mbedtls_ssl_config  mbedtls_ssl_config;

/* Defined in library/ssl_misc.h */
typedef struct mbedtls_ssl_transform mbedtls_ssl_transform;
typedef struct mbedtls_ssl_handshake_params mbedtls_ssl_handshake_params;
typedef struct mbedtls_ssl_sig_hash_set_t mbedtls_ssl_sig_hash_set_t;
typedef struct mbedtls_ssl_key_cert mbedtls_ssl_key_cert;

/**
 * \brief          Callback type: server-side session cache getter
 *
 *                 The session cache is logically a key value store, with
 *                 keys being session IDs and values being instances of
 *                 mbedtls_ssl_session.
 *
 *                 This callback retrieves an entry in this key-value store.
 *
 * \param data            The address of the session cache structure to query.
 * \param session_id      The buffer holding the session ID to query.
 * \param session_id_len  The length of \p session_id in Bytes.
 * \param session         The address of the session structure to populate.
 *                        It is initialized with mbdtls_ssl_session_init(),
 *                        and the callback must always leave it in a state
 *                        where it can safely be freed via
 *                        mbedtls_ssl_session_free() independent of the
 *                        return code of this function.
 *
 * \return                \c 0 on success
 * \return                A non-zero return value on failure.
 *
 */
typedef int mbedtls_ssl_cache_get_t(void *data,
                                    unsigned char const *session_id,
                                    size_t session_id_len,
                                    mbedtls_ssl_session *session);
/**
 * \brief          Callback type: server-side session cache setter
 *
 *                 The session cache is logically a key value store, with
 *                 keys being session IDs and values being instances of
 *                 mbedtls_ssl_session.
 *
 *                 This callback sets an entry in this key-value store.
 *
 * \param data            The address of the session cache structure to modify.
 * \param session_id      The buffer holding the session ID to query.
 * \param session_id_len  The length of \p session_id in Bytes.
 * \param session         The address of the session to be stored in the
 *                        session cache.
 *
 * \return                \c 0 on success
 * \return                A non-zero return value on failure.
 */
typedef int mbedtls_ssl_cache_set_t(void *data,
                                    unsigned char const *session_id,
                                    size_t session_id_len,
                                    const mbedtls_ssl_session *session);

/** Human-friendly representation of the (D)TLS protocol version. */
typedef enum {
    MBEDTLS_SSL_VERSION_UNKNOWN, /*!< Context not in use or version not yet negotiated. */
    MBEDTLS_SSL_VERSION_TLS1_2 = 0x0303, /*!< (D)TLS 1.2 */
    MBEDTLS_SSL_VERSION_TLS1_3 = 0x0304, /*!< (D)TLS 1.3 */
} mbedtls_ssl_protocol_version;

/*
 * This structure is used for storing current session data.
 *
 * Note: when changing this definition, we need to check and update:
 *  - in tests/suites/test_suite_ssl.function:
 *      ssl_populate_session() and ssl_serialize_session_save_load()
 *  - in library/ssl_tls.c:
 *      mbedtls_ssl_session_init() and mbedtls_ssl_session_free()
 *      mbedtls_ssl_session_save() and ssl_session_load()
 *      ssl_session_copy()
 */
struct mbedtls_ssl_session {

/*!< RecordSizeLimit received from the peer */

    unsigned char MBEDTLS_PRIVATE(exported);
    uint8_t MBEDTLS_PRIVATE(endpoint);          /*!< 0: client, 1: server */

    /** TLS version negotiated in the session. Used if and when renegotiating
     *  or resuming a session instead of the configured minor TLS version.
     */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(tls_version);

    mbedtls_time_t MBEDTLS_PRIVATE(start);       /*!< start time of current session */
    int MBEDTLS_PRIVATE(ciphersuite);            /*!< chosen ciphersuite */
    size_t MBEDTLS_PRIVATE(id_len);              /*!< session id length  */
    unsigned char MBEDTLS_PRIVATE(id)[32];       /*!< session identifier */
    unsigned char MBEDTLS_PRIVATE(master)[48];   /*!< the master secret  */

    mbedtls_x509_crt *MBEDTLS_PRIVATE(peer_cert);       /*!< peer X.509 cert chain */
    uint32_t MBEDTLS_PRIVATE(verify_result);          /*!<  verification result     */

};

/*
 * Identifiers for PRFs used in various versions of TLS.
 */
typedef enum {
    MBEDTLS_SSL_TLS_PRF_NONE,
    MBEDTLS_SSL_TLS_PRF_SHA384,
    MBEDTLS_SSL_TLS_PRF_SHA256,
    MBEDTLS_SSL_HKDF_EXPAND_SHA384,
    MBEDTLS_SSL_HKDF_EXPAND_SHA256
}
mbedtls_tls_prf_types;

typedef enum {
    MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET = 0,
} mbedtls_ssl_key_export_type;

/**
 * \brief           Callback type: Export key alongside random values for
 *                                 session identification, and PRF for
 *                                 implementation of TLS key exporters.
 *
 * \param p_expkey   Context for the callback.
 * \param type       The type of the key that is being exported.
 * \param secret     The address of the buffer holding the secret
 *                   that's being exporterd.
 * \param secret_len The length of \p secret in bytes.
 * \param client_random The client random bytes.
 * \param server_random The server random bytes.
 * \param tls_prf_type The identifier for the PRF used in the handshake
 *                     to which the key belongs.
 */
typedef void mbedtls_ssl_export_keys_t(void *p_expkey,
                                       mbedtls_ssl_key_export_type type,
                                       const unsigned char *secret,
                                       size_t secret_len,
                                       const unsigned char client_random[32],
                                       const unsigned char server_random[32],
                                       mbedtls_tls_prf_types tls_prf_type);

/* A type for storing user data in a library structure.
 *
 * The representation of type may change in future versions of the library.
 * Only the behaviors guaranteed by documented accessor functions are
 * guaranteed to remain stable.
 */
typedef union {
    uintptr_t n;                /* typically a handle to an associated object */
    void *p;                    /* typically a pointer to extra data */
} mbedtls_ssl_user_data_t;

/**
 * SSL/TLS configuration to be shared between mbedtls_ssl_context structures.
 */
struct mbedtls_ssl_config {
    /* Group items mostly by size. This helps to reduce memory wasted to
     * padding. It also helps to keep smaller fields early in the structure,
     * so that elements tend to be in the 128-element direct access window
     * on Arm Thumb, which reduces the code size. */

    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(max_tls_version);  /*!< max. TLS version used */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(min_tls_version);  /*!< min. TLS version used */

    /*
     * Flags (could be bit-fields to save RAM, but separate bytes make
     * the code smaller on architectures with an instruction for direct
     * byte access).
     */

    uint8_t MBEDTLS_PRIVATE(endpoint);      /*!< 0: client, 1: server               */
    uint8_t MBEDTLS_PRIVATE(transport);     /*!< 0: stream (TLS), 1: datagram (DTLS)    */
    uint8_t MBEDTLS_PRIVATE(authmode);      /*!< MBEDTLS_SSL_VERIFY_XXX             */
    /* needed even with renego disabled for LEGACY_BREAK_HANDSHAKE          */
    uint8_t MBEDTLS_PRIVATE(allow_legacy_renegotiation); /*!< MBEDTLS_LEGACY_XXX   */
    uint8_t MBEDTLS_PRIVATE(extended_ms);   /*!< negotiate extended master secret?  */

    /*
     * Pointers
     */

    /** Allowed ciphersuites for (D)TLS 1.2 (0-terminated)                  */
    const int *MBEDTLS_PRIVATE(ciphersuite_list);

    /** Callback for printing debug output                                  */
    void(*MBEDTLS_PRIVATE(f_dbg))(void *, int, const char *, int, const char *);
    void *MBEDTLS_PRIVATE(p_dbg);                    /*!< context for the debug function     */

    /** Callback for getting (pseudo-)random numbers                        */
    int(*MBEDTLS_PRIVATE(f_rng))(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_rng);                    /*!< context for the RNG function       */

    /** Callback to retrieve a session from the cache                       */
    mbedtls_ssl_cache_get_t *MBEDTLS_PRIVATE(f_get_cache);
    /** Callback to store a session into the cache                          */
    mbedtls_ssl_cache_set_t *MBEDTLS_PRIVATE(f_set_cache);
    void *MBEDTLS_PRIVATE(p_cache);                  /*!< context for cache callbacks        */

    /** Callback for setting cert according to SNI extension                */
    int(*MBEDTLS_PRIVATE(f_sni))(void *, mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_sni);                    /*!< context for SNI callback           */

    /** Callback to customize X.509 certificate chain verification          */
    int(*MBEDTLS_PRIVATE(f_vrfy))(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_vrfy);                   /*!< context for X.509 verify calllback */

    const mbedtls_x509_crt_profile *MBEDTLS_PRIVATE(cert_profile); /*!< verification profile */
    mbedtls_ssl_key_cert *MBEDTLS_PRIVATE(key_cert); /*!< own certificate/key pair(s)        */
    mbedtls_x509_crt *MBEDTLS_PRIVATE(ca_chain);     /*!< trusted CAs                        */
    mbedtls_x509_crl *MBEDTLS_PRIVATE(ca_crl);       /*!< trusted CAs CRLs                   */
#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
    mbedtls_x509_crt_ca_cb_t MBEDTLS_PRIVATE(f_ca_cb);
    void *MBEDTLS_PRIVATE(p_ca_cb);
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    const int *MBEDTLS_PRIVATE(sig_hashes);         /*!< allowed signature hashes           */
#endif
    const uint16_t *MBEDTLS_PRIVATE(sig_algs);      /*!< allowed signature algorithms       */
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if defined(MBEDTLS_ECP_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
    const mbedtls_ecp_group_id *MBEDTLS_PRIVATE(curve_list); /*!< allowed curves             */
#endif

    const uint16_t *MBEDTLS_PRIVATE(group_list);     /*!< allowed IANA NamedGroups */

    const char **MBEDTLS_PRIVATE(alpn_list);         /*!< ordered list of protocols          */

    /*
     * Numerical settings (int)
     */

    uint32_t MBEDTLS_PRIVATE(read_timeout);          /*!< timeout for mbedtls_ssl_read (ms)  */

    unsigned int MBEDTLS_PRIVATE(badmac_limit);      /*!< limit of records with a bad MAC    */

    /** User data pointer or handle.
     *
     * The library sets this to \p 0 when creating a context and does not
     * access it afterwards.
     */
    mbedtls_ssl_user_data_t MBEDTLS_PRIVATE(user_data);

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
    const mbedtls_x509_crt *MBEDTLS_PRIVATE(dn_hints);/*!< acceptable client cert issuers    */
#endif
};

struct mbedtls_ssl_context {
    const mbedtls_ssl_config *MBEDTLS_PRIVATE(conf); /*!< configuration information          */

    /*
     * Miscellaneous
     */
    int MBEDTLS_PRIVATE(state);                  /*!< SSL handshake: current state     */

    /**
     * Maximum TLS version to be negotiated, then negotiated TLS version.
     *
     * It is initialized as the configured maximum TLS version to be
     * negotiated by mbedtls_ssl_setup().
     *
     * When renegotiating or resuming a session, it is overwritten in the
     * ClientHello writing preparation stage with the previously negotiated
     * TLS version.
     *
     * On client side, it is updated to the TLS version selected by the server
     * for the handshake when the ServerHello is received.
     *
     * On server side, it is updated to the TLS version the server selects for
     * the handshake when the ClientHello is received.
     */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(tls_version);

    /** Multipurpose field.
     *
     * - DTLS: records with a bad MAC received.
     * - TLS: accumulated length of handshake fragments (up to \c in_hslen).
     *
     * This field is multipurpose in order to preserve the ABI in the
     * Mbed TLS 3.6 LTS branch. Until 3.6.2, it was only used in DTLS
     * and called `badmac_seen`.
     */
    unsigned MBEDTLS_PRIVATE(badmac_seen_or_in_hsfraglen);

    /** Callback to customize X.509 certificate chain verification          */
    int(*MBEDTLS_PRIVATE(f_vrfy))(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_vrfy);                   /*!< context for X.509 verify callback */

    mbedtls_ssl_send_t *MBEDTLS_PRIVATE(f_send); /*!< Callback for network send */
    mbedtls_ssl_recv_t *MBEDTLS_PRIVATE(f_recv); /*!< Callback for network receive */
    mbedtls_ssl_recv_timeout_t *MBEDTLS_PRIVATE(f_recv_timeout);
    /*!< Callback for network receive with timeout */

    void *MBEDTLS_PRIVATE(p_bio);                /*!< context for I/O operations   */

    /*
     * Session layer
     */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_in);            /*!<  current session data (in)   */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_out);           /*!<  current session data (out)  */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session);               /*!<  negotiated session data     */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_negotiate);     /*!<  session data in negotiation */

    mbedtls_ssl_handshake_params *MBEDTLS_PRIVATE(handshake);    /*!<  params required only during
                                                                    the handshake process        */

    /*
     * Record layer transformations
     */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_in);        /*!<  current transform params (in)
                                                                  *    This is always a reference,
                                                                  *    never an owning pointer.        */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_out);       /*!<  current transform params (out)
                                                                  *    This is always a reference,
                                                                  *    never an owning pointer.        */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform);           /*!<  negotiated transform params
                                                                  *    This pointer owns the transform
                                                                  *    it references.                  */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_negotiate); /*!<  transform params in negotiation
                                                                  *    This pointer owns the transform
                                                                  *    it references.                  */

    /*
     * Timers
     */
    void *MBEDTLS_PRIVATE(p_timer);              /*!< context for the timer callbacks */

    mbedtls_ssl_set_timer_t *MBEDTLS_PRIVATE(f_set_timer);       /*!< set timer callback */
    mbedtls_ssl_get_timer_t *MBEDTLS_PRIVATE(f_get_timer);       /*!< get timer callback */

    /*
     * Record layer (incoming data)
     */
    unsigned char *MBEDTLS_PRIVATE(in_buf);      /*!< input buffer                     */
    unsigned char *MBEDTLS_PRIVATE(in_ctr);      /*!< 64-bit incoming message counter
                                                    TLS: maintained by us
                                                    DTLS: read from peer             */
    unsigned char *MBEDTLS_PRIVATE(in_hdr);      /*!< start of record header           */
    unsigned char *MBEDTLS_PRIVATE(in_len);      /*!< two-bytes message length field   */
    unsigned char *MBEDTLS_PRIVATE(in_iv);       /*!< ivlen-byte IV                    */
    unsigned char *MBEDTLS_PRIVATE(in_msg);      /*!< message contents (in_iv+ivlen)   */
    unsigned char *MBEDTLS_PRIVATE(in_offt);     /*!< read offset in application data  */

    int MBEDTLS_PRIVATE(in_msgtype);             /*!< record header: message type      */
    size_t MBEDTLS_PRIVATE(in_msglen);           /*!< record header: message length    */
    size_t MBEDTLS_PRIVATE(in_left);             /*!< amount of data read so far       */

    size_t MBEDTLS_PRIVATE(in_hslen);            /*!< current handshake message length,
                                                    including the handshake header   */
    int MBEDTLS_PRIVATE(nb_zero);                /*!< # of 0-length encrypted messages */

    int MBEDTLS_PRIVATE(keep_current_message);   /*!< drop or reuse current message
                                                    on next call to record layer? */

    /* The following three variables indicate if and, if yes,
     * what kind of alert is pending to be sent.
     */
    unsigned char MBEDTLS_PRIVATE(send_alert);   /*!< Determines if a fatal alert
                                                    should be sent. Values:
                                                    - \c 0 , no alert is to be sent.
                                                    - \c 1 , alert is to be sent. */
    unsigned char MBEDTLS_PRIVATE(alert_type);   /*!< Type of alert if send_alert
                                                    != 0 */
    int MBEDTLS_PRIVATE(alert_reason);           /*!< The error code to be returned
                                                    to the user once the fatal alert
                                                    has been sent. */

    /*
     * Record layer (outgoing data)
     */
    unsigned char *MBEDTLS_PRIVATE(out_buf);     /*!< output buffer                    */
    unsigned char *MBEDTLS_PRIVATE(out_ctr);     /*!< 64-bit outgoing message counter  */
    unsigned char *MBEDTLS_PRIVATE(out_hdr);     /*!< start of record header           */
    unsigned char *MBEDTLS_PRIVATE(out_len);     /*!< two-bytes message length field   */
    unsigned char *MBEDTLS_PRIVATE(out_iv);      /*!< ivlen-byte IV                    */
    unsigned char *MBEDTLS_PRIVATE(out_msg);     /*!< message contents (out_iv+ivlen)  */

    int MBEDTLS_PRIVATE(out_msgtype);            /*!< record header: message type      */
    size_t MBEDTLS_PRIVATE(out_msglen);          /*!< record header: message length    */
    size_t MBEDTLS_PRIVATE(out_left);            /*!< amount of data not yet written   */

    unsigned char MBEDTLS_PRIVATE(cur_out_ctr)[MBEDTLS_SSL_SEQUENCE_NUMBER_LEN]; /*!<  Outgoing record sequence  number. */

    /*
     * User settings
     */
    /** Expected peer CN for verification.
     *
     * Also used on clients for SNI,
     * and for TLS 1.3 session resumption using tickets.
     *
     * The value of this field can be:
     * - \p NULL in a newly initialized or reset context.
     * - A heap-allocated copy of the last value passed to
     *   mbedtls_ssl_set_hostname(), if the last call had a non-null
     *  \p hostname argument.
     * - A special value to indicate that mbedtls_ssl_set_hostname()
     *   was called with \p NULL (as opposed to never having been called).
     *   See `mbedtls_ssl_get_hostname_pointer()` in `ssl_tls.c`.
     *
     * If this field contains the value \p NULL and the configuration option
     * #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
     * is unset, on a TLS client, attempting to verify a server certificate
     * results in the error
     * #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME.
     *
     * If this field contains the special value described above, or if
     * the value is \p NULL and the configuration option
     * #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
     * is set, then the peer name verification is skipped, which may be
     * insecure, especially on a client. Furthermore, on a client, the
     * server_name extension is not sent, and the server name is ignored
     * in TLS 1.3 session resumption using tickets.
     */
    char *MBEDTLS_PRIVATE(hostname);

    const char *MBEDTLS_PRIVATE(alpn_chosen);    /*!<  negotiated protocol                   */

    /*
     * Information for DTLS hello verify
     */

    /*
     * Secure renegotiation
     */
    /* needed to know when to send extension on server */
    int MBEDTLS_PRIVATE(secure_renegotiation);           /*!<  does peer support legacy or
                                                            secure renegotiation           */

    /** Callback to export key block and master secret                      */
    mbedtls_ssl_export_keys_t *MBEDTLS_PRIVATE(f_export_keys);
    void *MBEDTLS_PRIVATE(p_export_keys);            /*!< context for key export callback    */

    /** User data pointer or handle.
     *
     * The library sets this to \p 0 when creating a context and does not
     * access it afterwards.
     *
     * \warning Serializing and restoring an SSL context with
     *          mbedtls_ssl_context_save() and mbedtls_ssl_context_load()
     *          does not currently restore the user data.
     */
    mbedtls_ssl_user_data_t MBEDTLS_PRIVATE(user_data);
};

/**
 * \brief               Return the name of the ciphersuite associated with the
 *                      given ID
 *
 * \param ciphersuite_id SSL ciphersuite ID
 *
 * \return              a string containing the ciphersuite name
 */
const char *mbedtls_ssl_get_ciphersuite_name(const int ciphersuite_id);

/**
 * \brief          Initialize an SSL context
 *                 Just makes the context ready for mbedtls_ssl_setup() or
 *                 mbedtls_ssl_free()
 *
 * \param ssl      SSL context
 */
void mbedtls_ssl_init(mbedtls_ssl_context *ssl);

/**
 * \brief          Set up an SSL context for use
 *
 * \note           No copy of the configuration context is made, it can be
 *                 shared by many mbedtls_ssl_context structures.
 *
 * \warning        The conf structure will be accessed during the session.
 *                 It must not be modified or freed as long as the session
 *                 is active.
 *
 * \warning        This function must be called exactly once per context.
 *                 Calling mbedtls_ssl_setup again is not supported, even
 *                 if no session is active.
 *
 * \warning        After setting up a client context, if certificate-based
 *                 authentication is enabled, you should call
 *                 mbedtls_ssl_set_hostname() to specifiy the expected
 *                 name of the server. Without this, in most scenarios,
 *                 the TLS connection is insecure. See
 *                 #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *                 for more information.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param ssl      SSL context
 * \param conf     SSL configuration to use
 *
 * \return         0 if successful, or MBEDTLS_ERR_SSL_ALLOC_FAILED if
 *                 memory allocation failed
 */
int mbedtls_ssl_setup(mbedtls_ssl_context *ssl,
                      const mbedtls_ssl_config *conf, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Set the current endpoint type
 *
 * \param conf     SSL configuration
 * \param endpoint must be MBEDTLS_SSL_IS_CLIENT or MBEDTLS_SSL_IS_SERVER
 */
void mbedtls_ssl_conf_endpoint(mbedtls_ssl_config *conf, int endpoint);

/**
 * \brief          Get the current endpoint type
 *
 * \param conf     SSL configuration
 *
 * \return         Endpoint type, either MBEDTLS_SSL_IS_CLIENT
 *                 or MBEDTLS_SSL_IS_SERVER
 */
static inline int mbedtls_ssl_conf_get_endpoint(const mbedtls_ssl_config *conf)
{
    return conf->MBEDTLS_PRIVATE(endpoint);
}

/**
 * \brief           Set the transport type (TLS or DTLS).
 *                  Default: TLS
 *
 * \note            For DTLS, you must either provide a recv callback that
 *                  doesn't block, or one that handles timeouts, see
 *                  \c mbedtls_ssl_set_bio(). You also need to provide timer
 *                  callbacks with \c mbedtls_ssl_set_timer_cb().
 *
 * \param conf      SSL configuration
 * \param transport transport type:
 *                  MBEDTLS_SSL_TRANSPORT_STREAM for TLS,
 *                  MBEDTLS_SSL_TRANSPORT_DATAGRAM for DTLS.
 */
void mbedtls_ssl_conf_transport(mbedtls_ssl_config *conf, int transport);

/**
 * \brief          Set the certificate verification mode
 *                 Default: NONE on server, REQUIRED on client
 *
 * \param conf     SSL configuration
 * \param authmode can be:
 *
 *  MBEDTLS_SSL_VERIFY_NONE:      peer certificate is not checked
 *                        (default on server)
 *                        (insecure on client)
 *
 *  MBEDTLS_SSL_VERIFY_OPTIONAL:  peer certificate is checked, however the
 *                        handshake continues even if verification failed;
 *                        mbedtls_ssl_get_verify_result() can be called after the
 *                        handshake is complete.
 *
 *  MBEDTLS_SSL_VERIFY_REQUIRED:  peer *must* present a valid certificate,
 *                        handshake is aborted if verification failed.
 *                        (default on client)
 *
 * \note On client, MBEDTLS_SSL_VERIFY_REQUIRED is the recommended mode.
 * With MBEDTLS_SSL_VERIFY_OPTIONAL, the user needs to call mbedtls_ssl_get_verify_result() at
 * the right time(s), which may not be obvious, while REQUIRED always perform
 * the verification as soon as possible. For example, REQUIRED was protecting
 * against the "triple handshake" attack even before it was found.
 */
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode);

/**
 * \brief          Set the verification callback (Optional).
 *
 *                 If set, the provided verify callback is called for each
 *                 certificate in the peer's CRT chain, including the trusted
 *                 root. For more information, please see the documentation of
 *                 \c mbedtls_x509_crt_verify().
 *
 * \note           For per context callbacks and contexts, please use
 *                 mbedtls_ssl_set_verify() instead.
 *
 * \param conf     The SSL configuration to use.
 * \param f_vrfy   The verification callback to use during CRT verification.
 * \param p_vrfy   The opaque context to be passed to the callback.
 */
void mbedtls_ssl_conf_verify(mbedtls_ssl_config *conf,
                             int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                             void *p_vrfy);

/**
 * \brief          Set the random number generator callback
 *
 * \note           The callback with its parameter must remain valid as
 *                 long as there is an SSL context that uses the
 *                 SSL configuration.
 *
 * \param conf     SSL configuration
 * \param f_rng    RNG function (mandatory)
 * \param p_rng    RNG parameter
 */
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          mbedtls_f_rng_t *f_rng,
                          void *p_rng);

/**
 * \brief          Set the underlying BIO callbacks for write, read and
 *                 read-with-timeout.
 *
 * \param ssl      SSL context
 * \param p_bio    parameter (context) shared by BIO callbacks
 * \param f_send   write callback
 * \param f_recv   read callback
 * \param f_recv_timeout blocking read callback with timeout.
 *
 * \note           One of f_recv or f_recv_timeout can be NULL, in which case
 *                 the other is used. If both are non-NULL, f_recv_timeout is
 *                 used and f_recv is ignored (as if it were NULL).
 *
 * \note           The two most common use cases are:
 *                 - non-blocking I/O, f_recv != NULL, f_recv_timeout == NULL
 *                 - blocking I/O, f_recv == NULL, f_recv_timeout != NULL
 *
 * \note           For DTLS, you need to provide either a non-NULL
 *                 f_recv_timeout callback, or a f_recv that doesn't block.
 *
 * \note           See the documentations of \c mbedtls_ssl_send_t,
 *                 \c mbedtls_ssl_recv_t and \c mbedtls_ssl_recv_timeout_t for
 *                 the conventions those callbacks must follow.
 *
 * \note           On some platforms, net_sockets.c provides
 *                 \c mbedtls_net_send(), \c mbedtls_net_recv() and
 *                 \c mbedtls_net_recv_timeout() that are suitable to be used
 *                 here.
 */
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,
                         void *p_bio,
                         mbedtls_ssl_send_t *f_send,
                         mbedtls_ssl_recv_t *f_recv,
                         mbedtls_ssl_recv_timeout_t *f_recv_timeout);

/**
 * \brief           Callback type: generate and write session ticket
 *
 * \note            This describes what a callback implementation should do.
 *                  This callback should generate an encrypted and
 *                  authenticated ticket for the session and write it to the
 *                  output buffer. Here, ticket means the opaque ticket part
 *                  of the NewSessionTicket structure of RFC 5077.
 *
 * \param p_ticket  Context for the callback
 * \param session   SSL session to be written in the ticket
 * \param start     Start of the output buffer
 * \param end       End of the output buffer
 * \param tlen      On exit, holds the length written
 * \param lifetime  On exit, holds the lifetime of the ticket in seconds
 *
 * \return          0 if successful, or
 *                  a specific MBEDTLS_ERR_XXX code.
 */
typedef int mbedtls_ssl_ticket_write_t(void *p_ticket,
                                       const mbedtls_ssl_session *session,
                                       unsigned char *start,
                                       const unsigned char *end,
                                       size_t *tlen,
                                       uint32_t *lifetime);

/**
 * \brief           Callback type: parse and load session ticket
 *
 * \note            This describes what a callback implementation should do.
 *                  This callback should parse a session ticket as generated
 *                  by the corresponding mbedtls_ssl_ticket_write_t function,
 *                  and, if the ticket is authentic and valid, load the
 *                  session.
 *
 * \note            The implementation is allowed to modify the first len
 *                  bytes of the input buffer, eg to use it as a temporary
 *                  area for the decrypted ticket contents.
 *
 * \param p_ticket  Context for the callback
 * \param session   SSL session to be loaded
 * \param buf       Start of the buffer containing the ticket
 * \param len       Length of the ticket.
 *
 * \return          0 if successful, or
 *                  MBEDTLS_ERR_SSL_INVALID_MAC if not authentic, or
 *                  MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED if expired, or
 *                  any other non-zero code for other failures.
 */
typedef int mbedtls_ssl_ticket_parse_t(void *p_ticket,
                                       mbedtls_ssl_session *session,
                                       unsigned char *buf,
                                       size_t len);

/**
 * \brief          Get the session-id buffer.
 *
 * \param session  SSL session.
 *
 * \return         The address of the session-id buffer.
 */
static inline unsigned const char (*mbedtls_ssl_session_get_id(const mbedtls_ssl_session *
                                                               session))[32]
{
    return &session->MBEDTLS_PRIVATE(id);
}

/**
 * \brief          Callback type: generate a cookie
 *
 * \param ctx      Context for the callback
 * \param p        Buffer to write to,
 *                 must be updated to point right after the cookie
 * \param end      Pointer to one past the end of the output buffer
 * \param info     Client ID info that was passed to
 *                 \c mbedtls_ssl_set_client_transport_id()
 * \param ilen     Length of info in bytes
 *
 * \return         The callback must return 0 on success,
 *                 or a negative error code.
 */
typedef int mbedtls_ssl_cookie_write_t(void *ctx,
                                       unsigned char **p, unsigned char *end,
                                       const unsigned char *info, size_t ilen);

/**
 * \brief          Callback type: verify a cookie
 *
 * \param ctx      Context for the callback
 * \param cookie   Cookie to verify
 * \param clen     Length of cookie
 * \param info     Client ID info that was passed to
 *                 \c mbedtls_ssl_set_client_transport_id()
 * \param ilen     Length of info in bytes
 *
 * \return         The callback must return 0 if cookie is valid,
 *                 or a negative error code.
 */
typedef int mbedtls_ssl_cookie_check_t(void *ctx,
                                       const unsigned char *cookie, size_t clen,
                                       const unsigned char *info, size_t ilen);

/**
 * \brief          Set the data required to verify peer certificate
 *
 * \note           See \c mbedtls_x509_crt_verify() for notes regarding the
 *                 parameters ca_chain (maps to trust_ca for that function)
 *                 and ca_crl.
 *
 * \param conf     SSL configuration
 * \param ca_chain trusted CA chain (meaning all fully trusted top-level CAs)
 * \param ca_crl   trusted CA CRLs
 */
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf,
                               mbedtls_x509_crt *ca_chain,
                               mbedtls_x509_crl *ca_crl);

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)

#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

#endif

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
#if !defined(MBEDTLS_DEPRECATED_REMOVED) && defined(MBEDTLS_SSL_PROTO_TLS1_2)

#endif
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/**
 * \brief          Set or reset the hostname to check against the received
 *                 peer certificate. On a client, this also sets the
 *                 ServerName TLS extension, if that extension is enabled.
 *                 On a TLS 1.3 client, this also sets the server name in
 *                 the session resumption ticket, if that feature is enabled.
 *
 * \param ssl      SSL context
 * \param hostname The server hostname. This may be \c NULL to clear
 *                 the hostname.
 *
 * \note           Maximum hostname length #MBEDTLS_SSL_MAX_HOST_NAME_LEN.
 *
 * \note           If the hostname is \c NULL on a client, then the server
 *                 is not authenticated: it only needs to have a valid
 *                 certificate, not a certificate matching its name.
 *                 Therefore you should always call this function on a client,
 *                 unless the connection is set up to only allow
 *                 pre-shared keys, or in scenarios where server
 *                 impersonation is not a concern. See the documentation of
 *                 #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *                 for more details.
 *
 * \return         0 if successful, #MBEDTLS_ERR_SSL_ALLOC_FAILED on
 *                 allocation failure, #MBEDTLS_ERR_SSL_BAD_INPUT_DATA on
 *                 too long input hostname.
 *
 *                 Hostname set to the one provided on success (cleared
 *                 when NULL). On allocation failure hostname is cleared.
 *                 On too long input failure, old hostname is unchanged.
 */
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)

#endif

/**
 * \brief          Set the supported Application Layer Protocols.
 *
 * \param conf     SSL configuration
 * \param protos   Pointer to a NULL-terminated list of supported protocols,
 *                 in decreasing preference order. The pointer to the list is
 *                 recorded by the library for later reference as required, so
 *                 the lifetime of the table must be at least as long as the
 *                 lifetime of the SSL configuration structure.
 *
 * \return         0 on success, or MBEDTLS_ERR_SSL_BAD_INPUT_DATA.
 */
int mbedtls_ssl_conf_alpn_protocols(mbedtls_ssl_config *conf, const char **protos, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Get the name of the negotiated Application Layer Protocol.
 *                 This function should be called after the handshake is
 *                 completed.
 *
 * \param ssl      SSL context
 *
 * \return         Protocol name, or NULL if no protocol was negotiated.
 */
const char *mbedtls_ssl_get_alpn_protocol(const mbedtls_ssl_context *ssl);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

#endif /* MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Set the maximum supported version sent from the client side
 *                 and/or accepted at the server side.
 *
 * \note           After the handshake, you can call
 *                 mbedtls_ssl_get_version_number() to see what version was
 *                 negotiated.
 *
 * \param conf         SSL configuration
 * \param tls_version  TLS protocol version number (\c mbedtls_ssl_protocol_version)
 *                     (#MBEDTLS_SSL_VERSION_UNKNOWN is not valid)
 */
static inline void mbedtls_ssl_conf_max_tls_version(mbedtls_ssl_config *conf,
                                                    mbedtls_ssl_protocol_version tls_version)
{
    conf->MBEDTLS_PRIVATE(max_tls_version) = tls_version;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

#endif /* MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Set the minimum supported version sent from the client side
 *                 and/or accepted at the server side.
 *
 * \note           After the handshake, you can call
 *                 mbedtls_ssl_get_version_number() to see what version was
 *                 negotiated.
 *
 * \param conf         SSL configuration
 * \param tls_version  TLS protocol version number (\c mbedtls_ssl_protocol_version)
 *                     (#MBEDTLS_SSL_VERSION_UNKNOWN is not valid)
 */
static inline void mbedtls_ssl_conf_min_tls_version(mbedtls_ssl_config *conf,
                                                    mbedtls_ssl_protocol_version tls_version)
{
    conf->MBEDTLS_PRIVATE(min_tls_version) = tls_version;
}

/**
 * \brief          Return the result of the certificate verification
 *
 * \param ssl      The SSL context to use.
 *
 * \return         \c 0 if the certificate verification was successful.
 * \return         \c -1u if the result is not available. This may happen
 *                 e.g. if the handshake aborts early, or a verification
 *                 callback returned a fatal error.
 * \return         A bitwise combination of \c MBEDTLS_X509_BADCERT_XXX
 *                 and \c MBEDTLS_X509_BADCRL_XXX failure flags; see x509.h.
 */
uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the current maximum outgoing record payload in bytes.
 *
 * \note           The logic to determine the maximum outgoing record payload is
 *                 version-specific. It takes into account various factors, such as
 *                 the mbedtls_config.h setting \c MBEDTLS_SSL_OUT_CONTENT_LEN, extensions
 *                 such as the max fragment length or record size limit extension if
 *                 used, and for DTLS the path MTU as configured and current
 *                 record expansion.
 *
 * \note           With DTLS, \c mbedtls_ssl_write() will return an error if
 *                 called with a larger length value.
 *                 With TLS, \c mbedtls_ssl_write() will fragment the input if
 *                 necessary and return the number of bytes written; it is up
 *                 to the caller to call \c mbedtls_ssl_write() again in
 *                 order to send the remaining bytes if any.
 *
 * \sa             mbedtls_ssl_get_max_out_record_payload()
 * \sa             mbedtls_ssl_get_record_expansion()
 *
 * \param ssl      SSL context
 *
 * \return         Current maximum payload for an outgoing record,
 *                 or a negative error code.
 */
int mbedtls_ssl_get_max_out_record_payload(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the peer certificate from the current connection.
 *
 * \param  ssl     The SSL context to use. This must be initialized and setup.
 *
 * \return         The current peer certificate, if available.
 *                 The returned certificate is owned by the SSL context and
 *                 is valid only until the next call to the SSL API.
 * \return         \c NULL if no peer certificate is available. This might
 *                 be because the chosen ciphersuite doesn't use CRTs
 *                 (PSK-based ciphersuites, for example), or because
 *                 #MBEDTLS_SSL_KEEP_PEER_CERTIFICATE has been disabled,
 *                 allowing the stack to free the peer's CRT to save memory.
 *
 * \note           For one-time inspection of the peer's certificate during
 *                 the handshake, consider registering an X.509 CRT verification
 *                 callback through mbedtls_ssl_conf_verify() instead of calling
 *                 this function. Using mbedtls_ssl_conf_verify() also comes at
 *                 the benefit of allowing you to influence the verification
 *                 process, for example by masking expected and tolerated
 *                 verification failures.
 *
 * \warning        You must not use the pointer returned by this function
 *                 after any further call to the SSL API, including
 *                 mbedtls_ssl_read() and mbedtls_ssl_write(); this is
 *                 because the pointer might change during renegotiation,
 *                 which happens transparently to the user.
 *                 If you want to use the certificate across API calls,
 *                 you must make a copy.
 */
const mbedtls_x509_crt *mbedtls_ssl_get_peer_cert(const mbedtls_ssl_context *ssl);

/**
 * \brief          Perform the SSL handshake
 *
 * \param ssl      SSL context
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED if DTLS is in use
 *                 and the client did not demonstrate reachability yet - in
 *                 this case you must stop using the context (see below).
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received. See the
 *                 documentation of mbedtls_ssl_read() for more information
 *                 about this error code.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 \c 0,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           If DTLS is in use, then you may choose to handle
 *                 #MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED specially for logging
 *                 purposes, as it is an expected return value rather than an
 *                 actual error, but you still need to reset/free the context.
 *
 * \note           Remarks regarding event-driven DTLS:
 *                 If the function returns #MBEDTLS_ERR_SSL_WANT_READ, no datagram
 *                 from the underlying transport layer is currently being processed,
 *                 and it is safe to idle until the timer or the underlying transport
 *                 signal a new event. This is not true for a successful handshake,
 *                 in which case the datagram of the underlying transport that is
 *                 currently being processed might or might not contain further
 *                 DTLS records.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *                 Otherwise, the handshake may call psa_crypto_init()
 *                 if a negotiation involving TLS 1.3 takes place (this may
 *                 be the case even if TLS 1.3 is offered but eventually
 *                 not selected).
 *
 * \note           In TLS, reception of fragmented handshake messages is
 *                 supported with some limitations (those limitations do
 *                 not apply to DTLS, where defragmentation is fully
 *                 supported):
 *                 - The first fragment of a handshake message must be
 *                   at least 4 bytes long.
 *                 - Non-handshake records must not be interleaved between
 *                   the fragments of a handshake message. (This is permitted
 *                   in TLS 1.2 but not in TLS 1.3, but Mbed TLS rejects it
 *                   even in TLS 1.2.)
 */
int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          After calling mbedtls_ssl_handshake() to start the SSL
 *                 handshake you can call this function to check whether the
 *                 handshake is over for a given SSL context. This function
 *                 should be also used to determine when to stop calling
 *                 mbedtls_handshake_step() for that context.
 *
 * \param ssl      SSL context
 *
 * \return         \c 1 if handshake is over, \c 0 if it is still ongoing.
 */
static inline int mbedtls_ssl_is_handshake_over(mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(state) >= MBEDTLS_SSL_HANDSHAKE_OVER;
}

/**
 * \brief          Perform a single step of the SSL handshake
 *
 * \note           The state of the context (ssl->state) will be at
 *                 the next state after this function returns \c 0. Do not
 *                 call this function if mbedtls_ssl_is_handshake_over()
 *                 returns \c 1.
 *
 * \warning        Whilst in the past you may have used direct access to the
 *                 context state (ssl->state) in order to ascertain when to
 *                 stop calling this function and although you can still do
 *                 so with something like ssl->MBEDTLS_PRIVATE(state) or by
 *                 defining MBEDTLS_ALLOW_PRIVATE_ACCESS, this is now
 *                 considered deprecated and could be broken in any future
 *                 release. If you still find you have good reason for such
 *                 direct access, then please do contact the team to explain
 *                 this (raise an issue or post to the mailing list), so that
 *                 we can add a solution to your problem that will be
 *                 guaranteed to work in the future.
 *
 * \param ssl      SSL context
 *
 * \return         See mbedtls_ssl_handshake().
 *
 * \warning        If this function returns something other than \c 0,
 *                 #MBEDTLS_ERR_SSL_WANT_READ, #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA, you must stop using
 *                 the SSL context for reading or writing, and either free it
 *                 or call \c mbedtls_ssl_session_reset() on it before
 *                 re-using it for a new connection; the current connection
 *                 must be closed.
 */
int mbedtls_ssl_handshake_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Read at most 'len' application data bytes
 *
 * \param ssl      SSL context
 * \param buf      buffer that will hold the data
 * \param len      maximum number of bytes to read
 *
 * \return         The (positive) number of bytes read if successful.
 * \return         \c 0 if the read end of the underlying transport was closed
 *                 without sending a CloseNotify beforehand, which might happen
 *                 because of various reasons (internal error of an underlying
 *                 stack, non-conformant peer not sending a CloseNotify and
 *                 such) - in this case you must stop using the context
 *                 (see below).
 * \return         #MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY if the underlying
 *                 transport is still functional, but the peer has
 *                 acknowledged to not send anything anymore.
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_CLIENT_RECONNECT if we're at the server
 *                 side of a DTLS connection and the client is initiating a
 *                 new connection using the same source port. See below.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received.
 *                 This error code is only returned on the client side. It is
 *                 only returned if handling of TLS 1.3 NewSessionTicket
 *                 messages has been enabled through
 *                 mbedtls_ssl_conf_tls13_enable_signal_new_session_tickets().
 *                 This error code indicates that a TLS 1.3 NewSessionTicket
 *                 message has been received and parsed successfully by the
 *                 client. The ticket data can be retrieved from the SSL
 *                 context by calling mbedtls_ssl_get_session(). It remains
 *                 available until the next call to mbedtls_ssl_read().
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 a positive value,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CLIENT_RECONNECT or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           When this function returns #MBEDTLS_ERR_SSL_CLIENT_RECONNECT
 *                 (which can only happen server-side), it means that a client
 *                 is initiating a new connection using the same source port.
 *                 You can either treat that as a connection close and wait
 *                 for the client to resend a ClientHello, or directly
 *                 continue with \c mbedtls_ssl_handshake() with the same
 *                 context (as it has been reset internally). Either way, you
 *                 must make sure this is seen by the application as a new
 *                 connection: application state, if any, should be reset, and
 *                 most importantly the identity of the client must be checked
 *                 again. WARNING: not validating the identity of the client
 *                 again, or not transmitting the new identity to the
 *                 application layer, would allow authentication bypass!
 *
 * \note           Remarks regarding event-driven DTLS:
 *                 - If the function returns #MBEDTLS_ERR_SSL_WANT_READ, no datagram
 *                   from the underlying transport layer is currently being processed,
 *                   and it is safe to idle until the timer or the underlying transport
 *                   signal a new event.
 *                 - This function may return MBEDTLS_ERR_SSL_WANT_READ even if data was
 *                   initially available on the underlying transport, as this data may have
 *                   been only e.g. duplicated messages or a renegotiation request.
 *                   Therefore, you must be prepared to receive MBEDTLS_ERR_SSL_WANT_READ even
 *                   when reacting to an incoming-data event from the underlying transport.
 *                 - On success, the datagram of the underlying transport that is currently
 *                   being processed may contain further DTLS records. You should call
 *                   \c mbedtls_ssl_check_pending to check for remaining records.
 *
 */
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Try to write exactly 'len' application data bytes
 *
 * \warning        This function will do partial writes in some cases. If the
 *                 return value is non-negative but less than length, the
 *                 function must be called again with updated arguments:
 *                 buf + ret, len - ret (if ret is the return value) until
 *                 it returns a value equal to the last 'len' argument.
 *
 * \param ssl      SSL context
 * \param buf      buffer holding the data
 * \param len      how many bytes must be written
 *
 * \return         The (non-negative) number of bytes actually written if
 *                 successful (may be less than \p len).
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received. See the
 *                 documentation of mbedtls_ssl_read() for more information
 *                 about this error code.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 a non-negative value,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           When this function returns #MBEDTLS_ERR_SSL_WANT_WRITE/READ,
 *                 it must be called later with the *same* arguments,
 *                 until it returns a value greater than or equal to 0. When
 *                 the function returns #MBEDTLS_ERR_SSL_WANT_WRITE there may be
 *                 some partial data in the output buffer, however this is not
 *                 yet sent.
 *
 * \note           If the requested length is greater than the maximum
 *                 fragment length (either the built-in limit or the one set
 *                 or negotiated with the peer), then:
 *                 - with TLS, less bytes than requested are written.
 *                 - with DTLS, MBEDTLS_ERR_SSL_BAD_INPUT_DATA is returned.
 *                 \c mbedtls_ssl_get_max_out_record_payload() may be used to
 *                 query the active maximum fragment length.
 *
 * \note           Attempting to write 0 bytes will result in an empty TLS
 *                 application record being sent.
 */
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Send an alert message
 *
 * \param ssl       SSL context
 * \param level     The alert level of the message
 *                  (MBEDTLS_SSL_ALERT_LEVEL_WARNING or MBEDTLS_SSL_ALERT_LEVEL_FATAL)
 * \param message   The alert message (SSL_ALERT_MSG_*)
 *
 * \return          0 if successful, or a specific SSL error code.
 *
 * \note           If this function returns something other than 0 or
 *                 MBEDTLS_ERR_SSL_WANT_READ/WRITE, you must stop using
 *                 the SSL context for reading or writing, and either free it or
 *                 call \c mbedtls_ssl_session_reset() on it before re-using it
 *                 for a new connection; the current connection must be closed.
 */
int mbedtls_ssl_send_alert_message(mbedtls_ssl_context *ssl,
                                   unsigned char level,
                                   unsigned char message, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Free referenced items in an SSL context and clear memory
 *
 * \param ssl      SSL context
 */
void mbedtls_ssl_free(mbedtls_ssl_context *ssl);

/**
 * \brief          Initialize an SSL configuration context
 *                 Just makes the context ready for
 *                 mbedtls_ssl_config_defaults() or mbedtls_ssl_config_free().
 *
 * \note           You need to call mbedtls_ssl_config_defaults() unless you
 *                 manually set all of the relevant fields yourself.
 *
 * \param conf     SSL configuration context
 */
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);

/**
 * \brief          Load reasonable default SSL configuration values.
 *                 (You need to call mbedtls_ssl_config_init() first.)
 *
 * \param conf     SSL configuration context
 * \param endpoint MBEDTLS_SSL_IS_CLIENT or MBEDTLS_SSL_IS_SERVER
 * \param transport MBEDTLS_SSL_TRANSPORT_STREAM for TLS, or
 *                  MBEDTLS_SSL_TRANSPORT_DATAGRAM for DTLS
 * \param preset   a MBEDTLS_SSL_PRESET_XXX value
 *
 * \note           See \c mbedtls_ssl_conf_transport() for notes on DTLS.
 *
 * \return         0 if successful, or
 *                 MBEDTLS_ERR_XXX_ALLOC_FAILED on memory allocation error.
 */
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,
                                int endpoint, int transport, int preset);

/**
 * \brief          Free an SSL configuration context
 *
 * \param conf     SSL configuration context
 */
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);

/**
 * \brief          Initialize SSL session structure
 *
 * \param session  SSL session
 */
void mbedtls_ssl_session_init(mbedtls_ssl_session *session);

/**
 * \brief          Free referenced items in an SSL session including the
 *                 peer certificate and clear memory
 *
 * \note           A session object can be freed even if the SSL context
 *                 that was used to retrieve the session is still in use.
 *
 * \param session  SSL session
 */
void mbedtls_ssl_session_free(mbedtls_ssl_session *session);

#if defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT)
/* Maximum value for key_len in mbedtls_ssl_export_keying material. Depending on the TLS
 * version and the negotiated ciphersuite, larger keys could in principle be exported,
 * but for simplicity, we define one limit that works in all cases. TLS 1.3 with SHA256
 * has the strictest limit: 255 blocks of SHA256 output, or 8160 bytes. */
#define MBEDTLS_SSL_EXPORT_MAX_KEY_LEN 8160

#endif
#ifdef __cplusplus
}
#endif

#endif /* ssl.h */



#define MBEDTLS_SSL_DEBUG_MSG(level, args)            do { } while (0)
#define MBEDTLS_SSL_DEBUG_RET(level, text, ret) MinTlsRecordDiagnostic(diagnostics, text, __FILE__, __LINE__, ret)
#define MBEDTLS_SSL_DEBUG_BUF(level, text, buf, len)  do { } while (0)
#define MBEDTLS_SSL_DEBUG_MPI(level, text, X)         do { } while (0)
#define MBEDTLS_SSL_DEBUG_ECP(level, text, X)         do { } while (0)
#define MBEDTLS_SSL_DEBUG_CRT(level, text, crt)       do { } while (0)
#define MBEDTLS_SSL_DEBUG_ECDH(level, ecdh, attr)     do { } while (0)

/**
 * \def MBEDTLS_PRINTF_ATTRIBUTE
 *
 * Mark a function as having printf attributes, and thus enable checking
 * via -wFormat and other flags. This does nothing on builds with compilers
 * that do not support the format attribute
 *
 * Module:  library/debug.c
 * Caller:
 *
 * This module provides debugging functions.
 */
#if defined(__has_attribute)
#if __has_attribute(format)
#if defined(__MINGW32__) && __USE_MINGW_ANSI_STDIO == 1
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((__format__(gnu_printf, string_index, first_to_check)))
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((format(printf, string_index, first_to_check)))
#endif
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif /* __has_attribute(format) */
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif

/**
 * \def MBEDTLS_PRINTF_SIZET
 *
 * MBEDTLS_PRINTF_xxx: Due to issues with older window compilers
 * and MinGW we need to define the printf specifier for size_t
 * and long long per platform.
 *
 * Module:  library/debug.c
 * Caller:
 *
 * This module provides debugging functions.
 */
#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900)
   #include <inttypes.h>
   #define MBEDTLS_PRINTF_SIZET     PRIuPTR
   #define MBEDTLS_PRINTF_LONGLONG  PRId64
#else
   #define MBEDTLS_PRINTF_SIZET     "zu"
   #define MBEDTLS_PRINTF_LONGLONG  "lld"
#endif \
    /* defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900) */

#if !defined(MBEDTLS_PRINTF_MS_TIME)
#include <inttypes.h>
#if !defined(PRId64)
#define MBEDTLS_PRINTF_MS_TIME MBEDTLS_PRINTF_LONGLONG
#else
#define MBEDTLS_PRINTF_MS_TIME PRId64
#endif
#endif /* MBEDTLS_PRINTF_MS_TIME */

#endif /* MBEDTLS_DEBUG_H */

/* ===== Retained debug_internal.h ===== */

/*
 * Copyright The Mbed TLS Contributors
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: retain failure diagnostics only.
 * Original Mbed TLS 3.6.7 file: library/debug_internal.h.
 */
#ifndef MBEDTLS_DEBUG_INTERNAL_H
#define MBEDTLS_DEBUG_INTERNAL_H


#endif

/* ===== Retained entropy_poll.h ===== */

/**
 * \file entropy_poll.h
 *
 * \brief Platform-specific and custom entropy polling functions
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/entropy_poll.h.
 */
#ifndef MBEDTLS_ENTROPY_POLL_H
#define MBEDTLS_ENTROPY_POLL_H


#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Default thresholds for built-in sources, in bytes
 */
#define MBEDTLS_ENTROPY_MIN_PLATFORM     32     /**< Minimum for platform source    */
#if !defined(MBEDTLS_ENTROPY_MIN_HARDWARE)
#define MBEDTLS_ENTROPY_MIN_HARDWARE     32     /**< Minimum for the hardware source */
#endif

/**
 * \brief           Platform-specific entropy poll callback
 */
int mbedtls_platform_entropy_poll(void *data,
                                  unsigned char *output, size_t len, size_t *olen, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* entropy_poll.h */

/* ===== Retained error.h ===== */

/**
 * \file error.h
 *
 * \brief Error to string translation
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/error.h.
 */
#ifndef MBEDTLS_ERROR_H
#define MBEDTLS_ERROR_H


#include <stddef.h>

/**
 * Error code layout.
 *
 * Currently we try to keep all error codes within the negative space of 16
 * bits signed integers to support all platforms (-0x0001 - -0x7FFF). In
 * addition we'd like to give two layers of information on the error if
 * possible.
 *
 * For that purpose the error codes are segmented in the following manner:
 *
 * 16 bit error code bit-segmentation
 *
 * 1 bit  - Unused (sign bit)
 * 3 bits - High level module ID
 * 5 bits - Module-dependent error code
 * 7 bits - Low level module errors
 *
 * For historical reasons, low-level error codes are divided in even and odd,
 * even codes were assigned first, and -1 is reserved for other errors.
 *
 * Low-level module errors (0x0002-0x007E, 0x0001-0x007F)
 *
 * Module   Nr  Codes assigned
 * ERROR     2  0x006E          0x0001
 * MPI       7  0x0002-0x0010
 * GCM       3  0x0012-0x0016   0x0013-0x0013
 * THREADING 3  0x001A-0x001E
 * AES       5  0x0020-0x0022   0x0021-0x0025
 * CAMELLIA  3  0x0024-0x0026   0x0027-0x0027
 * BASE64    2  0x002A-0x002C
 * OID       1  0x002E-0x002E   0x000B-0x000B
 * PADLOCK   1  0x0030-0x0030
 * DES       2  0x0032-0x0032   0x0033-0x0033
 * CTR_DBRG  4  0x0034-0x003A
 * ENTROPY   3  0x003C-0x0040   0x003D-0x003F
 * NET      13  0x0042-0x0052   0x0043-0x0049
 * ARIA      4  0x0058-0x005E
 * ASN1      7  0x0060-0x006C
 * CMAC      1  0x007A-0x007A
 * PBKDF2    1  0x007C-0x007C
 * HMAC_DRBG 4                  0x0003-0x0009
 * CCM       3                  0x000D-0x0011
 * MD5       1                  0x002F-0x002F
 * RIPEMD160 1                  0x0031-0x0031
 * SHA1      1                  0x0035-0x0035 0x0073-0x0073
 * SHA256    1                  0x0037-0x0037 0x0074-0x0074
 * SHA512    1                  0x0039-0x0039 0x0075-0x0075
 * SHA-3     1                  0x0076-0x0076
 * CHACHA20  3                  0x0051-0x0055
 * POLY1305  3                  0x0057-0x005B
 * CHACHAPOLY 2 0x0054-0x0056
 * PLATFORM  2  0x0070-0x0072
 * LMS       5  0x0011-0x0019
 *
 * High-level module nr (3 bits - 0x0...-0x7...)
 * Name      ID  Nr of Errors
 * PEM       1   9
 * PKCS#12   1   4 (Started from top)
 * X509      2   20
 * PKCS5     2   4 (Started from top)
 * DHM       3   11
 * PK        3   15 (Started from top)
 * RSA       4   11
 * ECP       4   10 (Started from top)
 * MD        5   5
 * HKDF      5   1 (Started from top)
 * PKCS7     5   12 (Started from 0x5300)
 * SSL       5   3 (Started from 0x5F00)
 * CIPHER    6   8 (Started from 0x6080)
 * SSL       6   22 (Started from top, plus 0x6000)
 * SSL       7   20 (Started from 0x7000, gaps at
 *                   0x7380, 0x7900-0x7980, 0x7A80-0x7E80)
 *
 * Module dependent error code (5 bits 0x.00.-0x.F8.)
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Generic error */
#define MBEDTLS_ERR_ERROR_GENERIC_ERROR       -0x0001
/** This is a bug in the library */
#define MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED -0x006E

/** Hardware accelerator failed */
#define MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED     -0x0070
/** The requested feature is not supported by the platform */
#define MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED -0x0072

/**
 * \brief Combines a high-level and low-level error code together.
 *
 *        Preserve the originating diagnostic while combining status codes.
 */
#define MBEDTLS_ERROR_ADD(high, low) \
    MinTlsCombineDiagnostic(diagnostics, high, low)

/**
 * \brief Translate an Mbed TLS error code into a string representation.
 *        The result is truncated if necessary and always includes a
 *        terminating null byte.
 *
 * \param errnum    error code
 * \param buffer    buffer to place representation in
 * \param buflen    length of the buffer
 */
void mbedtls_strerror(int errnum, char *buffer, size_t buflen);

/**
 * \brief Translate the high-level part of an Mbed TLS error code into a string
 *        representation.
 *
 * This function returns a const pointer to an un-modifiable string. The caller
 * must not try to modify the string. It is intended to be used mostly for
 * logging purposes.
 *
 * \param error_code    error code
 *
 * \return The string representation of the error code, or \c NULL if the error
 *         code is unknown.
 */
const char *mbedtls_high_level_strerr(int error_code);

/**
 * \brief Translate the low-level part of an Mbed TLS error code into a string
 *        representation.
 *
 * This function returns a const pointer to an un-modifiable string. The caller
 * must not try to modify the string. It is intended to be used mostly for
 * logging purposes.
 *
 * \param error_code    error code
 *
 * \return The string representation of the error code, or \c NULL if the error
 *         code is unknown.
 */
const char *mbedtls_low_level_strerr(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* error.h */

/* ===== Retained gcm.h ===== */

/**
 * \file gcm.h
 *
 * \brief This file contains GCM definitions and functions.
 *
 * The Galois/Counter Mode (GCM) for 128-bit block ciphers is defined
 * in <em>D. McGrew, J. Viega, The Galois/Counter Mode of Operation
 * (GCM), Natl. Inst. Stand. Technol.</em>
 *
 * For more information on GCM, see <em>NIST SP 800-38D: Recommendation for
 * Block Cipher Modes of Operation: Galois/Counter Mode (GCM) and GMAC</em>.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/gcm.h.
 */

#ifndef MBEDTLS_GCM_H
#define MBEDTLS_GCM_H



#include <stdint.h>

#define MBEDTLS_GCM_ENCRYPT     1
#define MBEDTLS_GCM_DECRYPT     0

/** Authenticated decryption failed. */
#define MBEDTLS_ERR_GCM_AUTH_FAILED                       -0x0012
/** Bad input parameters to function. */
#define MBEDTLS_ERR_GCM_BAD_INPUT                         -0x0014
/** An output buffer is too small. */
#define MBEDTLS_ERR_GCM_BUFFER_TOO_SMALL                  -0x0016

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_GCM_ALT)

#if defined(MBEDTLS_GCM_LARGE_TABLE)
#define MBEDTLS_GCM_HTABLE_SIZE 256
#else
#define MBEDTLS_GCM_HTABLE_SIZE 16
#endif

/**
 * \brief          The GCM context structure.
 */
typedef struct mbedtls_gcm_context {
    mbedtls_cipher_context_t MBEDTLS_PRIVATE(cipher_ctx);    /*!< The cipher context used. */
    uint64_t MBEDTLS_PRIVATE(H)[MBEDTLS_GCM_HTABLE_SIZE][2]; /*!< Precalculated HTable. */
    uint64_t MBEDTLS_PRIVATE(len);                           /*!< The total length of the encrypted data. */
    uint64_t MBEDTLS_PRIVATE(add_len);                       /*!< The total length of the additional data. */
    unsigned char MBEDTLS_PRIVATE(base_ectr)[16];            /*!< The first ECTR for tag. */
    unsigned char MBEDTLS_PRIVATE(y)[16];                    /*!< The Y working value. */
    unsigned char MBEDTLS_PRIVATE(buf)[16];                  /*!< The buf working value. */
    unsigned char MBEDTLS_PRIVATE(mode);                     /*!< The operation to perform:
                                                              #MBEDTLS_GCM_ENCRYPT or
                                                              #MBEDTLS_GCM_DECRYPT. */
    unsigned char MBEDTLS_PRIVATE(acceleration);             /*!< The acceleration to use. */
}
mbedtls_gcm_context;

#else
#include "gcm_alt.h"
#endif /* !MBEDTLS_GCM_ALT */

/**
 * \brief           This function initializes the specified GCM context,
 *                  to make references valid, and prepares the context
 *                  for mbedtls_gcm_setkey() or mbedtls_gcm_free().
 *
 *                  The function does not bind the GCM context to a particular
 *                  cipher, nor set the key. For this purpose, use
 *                  mbedtls_gcm_setkey().
 *
 * \param ctx       The GCM context to initialize. This must not be \c NULL.
 */
void mbedtls_gcm_init(mbedtls_gcm_context *ctx);

/**
 * \brief           This function associates a GCM context with a
 *                  cipher algorithm and a key.
 *
 * \param ctx       The GCM context. This must be initialized.
 * \param cipher    The 128-bit block cipher to use.
 * \param key       The encryption key. This must be a readable buffer of at
 *                  least \p keybits bits.
 * \param keybits   The key size in bits. Valid options are:
 *                  <ul><li>128 bits</li>
 *                  <li>192 bits</li>
 *                  <li>256 bits</li></ul>
 *
 * \return          \c 0 on success.
 * \return          A cipher-specific error code on failure.
 */
int mbedtls_gcm_setkey(mbedtls_gcm_context *ctx,
                       mbedtls_cipher_id_t cipher,
                       const unsigned char *key,
                       unsigned int keybits, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function performs GCM encryption or decryption of a buffer.
 *
 * \note            The output buffer \p output can be the same as the input
 *                  buffer \p input. If \p output is greater than \p input, they
 *                  cannot overlap.
 *
 * \warning         When this function performs a decryption, it outputs the
 *                  authentication tag and does not verify that the data is
 *                  authentic. You should use this function to perform encryption
 *                  only. For decryption, use mbedtls_gcm_auth_decrypt() instead.
 *
 * \param ctx       The GCM context to use for encryption or decryption. This
 *                  must be initialized.
 * \param mode      The operation to perform:
 *                  - #MBEDTLS_GCM_ENCRYPT to perform authenticated encryption.
 *                    The ciphertext is written to \p output and the
 *                    authentication tag is written to \p tag.
 *                  - #MBEDTLS_GCM_DECRYPT to perform decryption.
 *                    The plaintext is written to \p output and the
 *                    authentication tag is written to \p tag.
 *                    Note that this mode is not recommended, because it does
 *                    not verify the authenticity of the data. For this reason,
 *                    you should use mbedtls_gcm_auth_decrypt() instead of
 *                    calling this function in decryption mode.
 * \param length    The length of the input data, which is equal to the length
 *                  of the output data.
 * \param iv        The initialization vector. This must be a readable buffer of
 *                  at least \p iv_len Bytes.
 * \param iv_len    The length of the IV.
 * \param add       The buffer holding the additional data. This must be of at
 *                  least that size in Bytes.
 * \param add_len   The length of the additional data.
 * \param input     The buffer holding the input data. If \p length is greater
 *                  than zero, this must be a readable buffer of at least that
 *                  size in Bytes.
 * \param output    The buffer for holding the output data. If \p length is greater
 *                  than zero, this must be a writable buffer of at least that
 *                  size in Bytes.
 * \param tag_len   The length of the tag to generate.
 * \param tag       The buffer for holding the tag. This must be a writable
 *                  buffer of at least \p tag_len Bytes.
 *
 * \return          \c 0 if the encryption or decryption was performed
 *                  successfully. Note that in #MBEDTLS_GCM_DECRYPT mode,
 *                  this does not indicate that the data is authentic.
 * \return          #MBEDTLS_ERR_GCM_BAD_INPUT if the lengths or pointers are
 *                  not valid or a cipher-specific error code if the encryption
 *                  or decryption failed.
 */
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
                              int mode,
                              size_t length,
                              const unsigned char *iv,
                              size_t iv_len,
                              const unsigned char *add,
                              size_t add_len,
                              const unsigned char *input,
                              unsigned char *output,
                              size_t tag_len,
                              unsigned char *tag, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function performs a GCM authenticated decryption of a
 *                  buffer.
 *
 * \note            The output buffer \p output can be the same as the input
 *                  buffer \p input. If \p output is greater than \p input, they
 *                  cannot overlap. Implementations which require
 *                  MBEDTLS_GCM_ALT to be enabled may not provide support for
 *                  overlapping buffers.
 *
 * \param ctx       The GCM context. This must be initialized.
 * \param length    The length of the ciphertext to decrypt, which is also
 *                  the length of the decrypted plaintext.
 * \param iv        The initialization vector. This must be a readable buffer
 *                  of at least \p iv_len Bytes.
 * \param iv_len    The length of the IV.
 * \param add       The buffer holding the additional data. This must be of at
 *                  least that size in Bytes.
 * \param add_len   The length of the additional data.
 * \param tag       The buffer holding the tag to verify. This must be a
 *                  readable buffer of at least \p tag_len Bytes.
 * \param tag_len   The length of the tag to verify.
 * \param input     The buffer holding the ciphertext. If \p length is greater
 *                  than zero, this must be a readable buffer of at least that
 *                  size.
 * \param output    The buffer for holding the decrypted plaintext. If \p length
 *                  is greater than zero, this must be a writable buffer of at
 *                  least that size.
 *
 * \return          \c 0 if successful and authenticated.
 * \return          #MBEDTLS_ERR_GCM_AUTH_FAILED if the tag does not match.
 * \return          #MBEDTLS_ERR_GCM_BAD_INPUT if the lengths or pointers are
 *                  not valid or a cipher-specific error code if the decryption
 *                  failed.
 */
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *ctx,
                             size_t length,
                             const unsigned char *iv,
                             size_t iv_len,
                             const unsigned char *add,
                             size_t add_len,
                             const unsigned char *tag,
                             size_t tag_len,
                             const unsigned char *input,
                             unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function starts a GCM encryption or decryption
 *                  operation.
 *
 * \param ctx       The GCM context. This must be initialized.
 * \param mode      The operation to perform: #MBEDTLS_GCM_ENCRYPT or
 *                  #MBEDTLS_GCM_DECRYPT.
 * \param iv        The initialization vector. This must be a readable buffer of
 *                  at least \p iv_len Bytes.
 * \param iv_len    The length of the IV.
 *
 * \return          \c 0 on success.
 */
int mbedtls_gcm_starts(mbedtls_gcm_context *ctx,
                       int mode,
                       const unsigned char *iv,
                       size_t iv_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function feeds an input buffer as associated data
 *                  (authenticated but not encrypted data) in a GCM
 *                  encryption or decryption operation.
 *
 *                  Call this function after mbedtls_gcm_starts() to pass
 *                  the associated data. If the associated data is empty,
 *                  you do not need to call this function. You may not
 *                  call this function after calling mbedtls_cipher_update().
 *
 * \param ctx       The GCM context. This must have been started with
 *                  mbedtls_gcm_starts() and must not have yet received
 *                  any input with mbedtls_gcm_update().
 * \param add       The buffer holding the additional data, or \c NULL
 *                  if \p add_len is \c 0.
 * \param add_len   The length of the additional data. If \c 0,
 *                  \p add may be \c NULL.
 *
 * \return          \c 0 on success.
 */
int mbedtls_gcm_update_ad(mbedtls_gcm_context *ctx,
                          const unsigned char *add,
                          size_t add_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function feeds an input buffer into an ongoing GCM
 *                  encryption or decryption operation.
 *
 *                  You may call this function zero, one or more times
 *                  to pass successive parts of the input: the plaintext to
 *                  encrypt, or the ciphertext (not including the tag) to
 *                  decrypt. After the last part of the input, call
 *                  mbedtls_gcm_finish().
 *
 *                  This function may produce output in one of the following
 *                  ways:
 *                  - Immediate output: the output length is always equal
 *                    to the input length.
 *                  - Buffered output: the output consists of a whole number
 *                    of 16-byte blocks. If the total input length so far
 *                    (not including associated data) is 16 \* *B* + *A*
 *                    with *A* < 16 then the total output length is 16 \* *B*.
 *
 *                  In particular:
 *                  - It is always correct to call this function with
 *                    \p output_size >= \p input_length + 15.
 *                  - If \p input_length is a multiple of 16 for all the calls
 *                    to this function during an operation, then it is
 *                    correct to use \p output_size = \p input_length.
 *
 * \note            The output buffer \p output can be the same as the input
 *                  buffer \p input. If \p output is greater than \p input, they
 *                  cannot overlap. Implementations which require
 *                  MBEDTLS_GCM_ALT to be enabled may not provide support for
 *                  overlapping buffers.
 *
 * \param ctx           The GCM context. This must be initialized.
 * \param input         The buffer holding the input data. If \p input_length
 *                      is greater than zero, this must be a readable buffer
 *                      of at least \p input_length bytes.
 * \param input_length  The length of the input data in bytes.
 * \param output        The buffer for the output data. If \p output_size
 *                      is greater than zero, this must be a writable buffer of
 *                      of at least \p output_size bytes.
 * \param output_size   The size of the output buffer in bytes.
 *                      See the function description regarding the output size.
 * \param output_length On success, \p *output_length contains the actual
 *                      length of the output written in \p output.
 *                      On failure, the content of \p *output_length is
 *                      unspecified.
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_GCM_BAD_INPUT on failure:
 *                 total input length too long,
 *                 unsupported input/output buffer overlap detected,
 *                 or \p output_size too small.
 */
int mbedtls_gcm_update(mbedtls_gcm_context *ctx,
                       const unsigned char *input, size_t input_length,
                       unsigned char *output, size_t output_size,
                       size_t *output_length, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function finishes the GCM operation and generates
 *                  the authentication tag.
 *
 *                  It wraps up the GCM stream, and generates the
 *                  tag. The tag can have a maximum length of 16 Bytes.
 *
 * \param ctx       The GCM context. This must be initialized.
 * \param tag       The buffer for holding the tag. This must be a writable
 *                  buffer of at least \p tag_len Bytes.
 * \param tag_len   The length of the tag to generate. This must be at least
 *                  four.
 * \param output    The buffer for the final output.
 *                  If \p output_size is nonzero, this must be a writable
 *                  buffer of at least \p output_size bytes.
 * \param output_size  The size of the \p output buffer in bytes.
 *                  This must be large enough for the output that
 *                  mbedtls_gcm_update() has not produced. In particular:
 *                  - If mbedtls_gcm_update() produces immediate output,
 *                    or if the total input size is a multiple of \c 16,
 *                    then mbedtls_gcm_finish() never produces any output,
 *                    so \p output_size can be \c 0.
 *                  - \p output_size never needs to be more than \c 15.
 * \param output_length On success, \p *output_length contains the actual
 *                      length of the output written in \p output.
 *                      On failure, the content of \p *output_length is
 *                      unspecified.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_GCM_BAD_INPUT on failure:
 *                  invalid value of \p tag_len,
 *                  or \p output_size too small.
 */
int mbedtls_gcm_finish(mbedtls_gcm_context *ctx,
                       unsigned char *output, size_t output_size,
                       size_t *output_length,
                       unsigned char *tag, size_t tag_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           This function clears a GCM context and the underlying
 *                  cipher sub-context.
 *
 * \param ctx       The GCM context to clear. If this is \c NULL, the call has
 *                  no effect. Otherwise, this must be initialized.
 */
void mbedtls_gcm_free(mbedtls_gcm_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* gcm.h */

/* ===== Retained md_wrap.h ===== */

/**
 * \file md_wrap.h
 *
 * \brief Message digest wrappers.
 *
 * \warning This in an internal header. Do not include directly.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/md_wrap.h.
 */
#ifndef MBEDTLS_MD_WRAP_H
#define MBEDTLS_MD_WRAP_H



#ifdef __cplusplus
extern "C" {
#endif

/**
 * Message digest information.
 * Allows message digest functions to be called in a generic way.
 */
struct mbedtls_md_info_t {
    /** Digest identifier */
    mbedtls_md_type_t type;

    /** Output length of the digest function in bytes */
    unsigned char size;

    /** Block length of the digest function in bytes */
    unsigned char block_size;
};

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_MD_WRAP_H */

/* ===== Retained oid.h ===== */

/**
 * \file oid.h
 *
 * \brief Object Identifier (OID) database
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/oid.h.
 */
#ifndef MBEDTLS_OID_H
#define MBEDTLS_OID_H



#include <stddef.h>



/** OID is not found. */
#define MBEDTLS_ERR_OID_NOT_FOUND                         -0x002E
/** output buffer is too small */
#define MBEDTLS_ERR_OID_BUF_TOO_SMALL                     -0x000B

/* This is for the benefit of X.509, but defined here in order to avoid
 * having a "backwards" include of x.509.h here */
/*
 * X.509 extension types (internal, arbitrary values for bitsets)
 */
#define MBEDTLS_OID_X509_EXT_AUTHORITY_KEY_IDENTIFIER    (1 << 0)
#define MBEDTLS_OID_X509_EXT_SUBJECT_KEY_IDENTIFIER      (1 << 1)
#define MBEDTLS_OID_X509_EXT_KEY_USAGE                   (1 << 2)
#define MBEDTLS_OID_X509_EXT_CERTIFICATE_POLICIES        (1 << 3)
#define MBEDTLS_OID_X509_EXT_POLICY_MAPPINGS             (1 << 4)
#define MBEDTLS_OID_X509_EXT_SUBJECT_ALT_NAME            (1 << 5)
#define MBEDTLS_OID_X509_EXT_ISSUER_ALT_NAME             (1 << 6)
#define MBEDTLS_OID_X509_EXT_SUBJECT_DIRECTORY_ATTRS     (1 << 7)
#define MBEDTLS_OID_X509_EXT_BASIC_CONSTRAINTS           (1 << 8)
#define MBEDTLS_OID_X509_EXT_NAME_CONSTRAINTS            (1 << 9)
#define MBEDTLS_OID_X509_EXT_POLICY_CONSTRAINTS          (1 << 10)
#define MBEDTLS_OID_X509_EXT_EXTENDED_KEY_USAGE          (1 << 11)
#define MBEDTLS_OID_X509_EXT_CRL_DISTRIBUTION_POINTS     (1 << 12)
#define MBEDTLS_OID_X509_EXT_INIHIBIT_ANYPOLICY          (1 << 13)
#define MBEDTLS_OID_X509_EXT_FRESHEST_CRL                (1 << 14)
#define MBEDTLS_OID_X509_EXT_NS_CERT_TYPE                (1 << 16)

/*
 * Maximum number of OID components allowed
 */
#define MBEDTLS_OID_MAX_COMPONENTS              128

/*
 * Top level OID tuples
 */
#define MBEDTLS_OID_ISO_MEMBER_BODIES           "\x2a"          /* {iso(1) member-body(2)} */
#define MBEDTLS_OID_ISO_IDENTIFIED_ORG          "\x2b"          /* {iso(1) identified-organization(3)} */
#define MBEDTLS_OID_ISO_CCITT_DS                "\x55"          /* {joint-iso-ccitt(2) ds(5)} */
#define MBEDTLS_OID_ISO_ITU_COUNTRY             "\x60"          /* {joint-iso-itu-t(2) country(16)} */

/*
 * ISO Member bodies OID parts
 */
#define MBEDTLS_OID_COUNTRY_US                  "\x86\x48"      /* {us(840)} */
#define MBEDTLS_OID_ORG_RSA_DATA_SECURITY       "\x86\xf7\x0d"  /* {rsadsi(113549)} */
#define MBEDTLS_OID_RSA_COMPANY                 MBEDTLS_OID_ISO_MEMBER_BODIES MBEDTLS_OID_COUNTRY_US \
        MBEDTLS_OID_ORG_RSA_DATA_SECURITY                                 /* {iso(1) member-body(2) us(840) rsadsi(113549)} */
#define MBEDTLS_OID_ORG_ANSI_X9_62              "\xce\x3d" /* ansi-X9-62(10045) */
#define MBEDTLS_OID_ANSI_X9_62                  MBEDTLS_OID_ISO_MEMBER_BODIES MBEDTLS_OID_COUNTRY_US \
        MBEDTLS_OID_ORG_ANSI_X9_62

/*
 * ISO Identified organization OID parts
 */
#define MBEDTLS_OID_ORG_DOD                     "\x06"          /* {dod(6)} */
#define MBEDTLS_OID_ORG_OIW                     "\x0e"
#define MBEDTLS_OID_OIW_SECSIG                  MBEDTLS_OID_ORG_OIW "\x03"
#define MBEDTLS_OID_OIW_SECSIG_ALG              MBEDTLS_OID_OIW_SECSIG "\x02"
#define MBEDTLS_OID_OIW_SECSIG_SHA1             MBEDTLS_OID_OIW_SECSIG_ALG "\x1a"
#define MBEDTLS_OID_ORG_THAWTE                  "\x65"          /* thawte(101) */
#define MBEDTLS_OID_THAWTE                      MBEDTLS_OID_ISO_IDENTIFIED_ORG \
        MBEDTLS_OID_ORG_THAWTE
#define MBEDTLS_OID_ORG_CERTICOM                "\x81\x04"  /* certicom(132) */
#define MBEDTLS_OID_CERTICOM                    MBEDTLS_OID_ISO_IDENTIFIED_ORG \
        MBEDTLS_OID_ORG_CERTICOM
#define MBEDTLS_OID_ORG_TELETRUST               "\x24" /* teletrust(36) */
#define MBEDTLS_OID_TELETRUST                   MBEDTLS_OID_ISO_IDENTIFIED_ORG \
        MBEDTLS_OID_ORG_TELETRUST

/*
 * ISO ITU OID parts
 */
#define MBEDTLS_OID_ORGANIZATION                "\x01"          /* {organization(1)} */
#define MBEDTLS_OID_ISO_ITU_US_ORG              MBEDTLS_OID_ISO_ITU_COUNTRY MBEDTLS_OID_COUNTRY_US \
        MBEDTLS_OID_ORGANIZATION                                                                                            /* {joint-iso-itu-t(2) country(16) us(840) organization(1)} */

#define MBEDTLS_OID_ORG_GOV                     "\x65"          /* {gov(101)} */
#define MBEDTLS_OID_GOV                         MBEDTLS_OID_ISO_ITU_US_ORG MBEDTLS_OID_ORG_GOV /* {joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101)} */

#define MBEDTLS_OID_ORG_NETSCAPE                "\x86\xF8\x42"  /* {netscape(113730)} */
#define MBEDTLS_OID_NETSCAPE                    MBEDTLS_OID_ISO_ITU_US_ORG MBEDTLS_OID_ORG_NETSCAPE /* Netscape OID {joint-iso-itu-t(2) country(16) us(840) organization(1) netscape(113730)} */

/* ISO arc for standard certificate and CRL extensions */
#define MBEDTLS_OID_ID_CE                       MBEDTLS_OID_ISO_CCITT_DS "\x1D" /**< id-ce OBJECT IDENTIFIER  ::=  {joint-iso-ccitt(2) ds(5) 29} */

#define MBEDTLS_OID_NIST_ALG                    MBEDTLS_OID_GOV "\x03\x04" /** { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithm(4) */

/**
 * Private Internet Extensions
 * { iso(1) identified-organization(3) dod(6) internet(1)
 *                      security(5) mechanisms(5) pkix(7) }
 */
#define MBEDTLS_OID_INTERNET                    MBEDTLS_OID_ISO_IDENTIFIED_ORG MBEDTLS_OID_ORG_DOD \
    "\x01"
#define MBEDTLS_OID_PKIX                        MBEDTLS_OID_INTERNET "\x05\x05\x07"

/*
 * Arc for standard naming attributes
 */
#define MBEDTLS_OID_AT                          MBEDTLS_OID_ISO_CCITT_DS "\x04" /**< id-at OBJECT IDENTIFIER ::= {joint-iso-ccitt(2) ds(5) 4} */
#define MBEDTLS_OID_AT_CN                       MBEDTLS_OID_AT "\x03" /**< id-at-commonName AttributeType:= {id-at 3} */
#define MBEDTLS_OID_AT_SUR_NAME                 MBEDTLS_OID_AT "\x04" /**< id-at-surName AttributeType:= {id-at 4} */
#define MBEDTLS_OID_AT_SERIAL_NUMBER            MBEDTLS_OID_AT "\x05" /**< id-at-serialNumber AttributeType:= {id-at 5} */
#define MBEDTLS_OID_AT_COUNTRY                  MBEDTLS_OID_AT "\x06" /**< id-at-countryName AttributeType:= {id-at 6} */
#define MBEDTLS_OID_AT_LOCALITY                 MBEDTLS_OID_AT "\x07" /**< id-at-locality AttributeType:= {id-at 7} */
#define MBEDTLS_OID_AT_STATE                    MBEDTLS_OID_AT "\x08" /**< id-at-state AttributeType:= {id-at 8} */
#define MBEDTLS_OID_AT_ORGANIZATION             MBEDTLS_OID_AT "\x0A" /**< id-at-organizationName AttributeType:= {id-at 10} */
#define MBEDTLS_OID_AT_ORG_UNIT                 MBEDTLS_OID_AT "\x0B" /**< id-at-organizationalUnitName AttributeType:= {id-at 11} */
#define MBEDTLS_OID_AT_TITLE                    MBEDTLS_OID_AT "\x0C" /**< id-at-title AttributeType:= {id-at 12} */
#define MBEDTLS_OID_AT_POSTAL_ADDRESS           MBEDTLS_OID_AT "\x10" /**< id-at-postalAddress AttributeType:= {id-at 16} */
#define MBEDTLS_OID_AT_POSTAL_CODE              MBEDTLS_OID_AT "\x11" /**< id-at-postalCode AttributeType:= {id-at 17} */
#define MBEDTLS_OID_AT_GIVEN_NAME               MBEDTLS_OID_AT "\x2A" /**< id-at-givenName AttributeType:= {id-at 42} */
#define MBEDTLS_OID_AT_INITIALS                 MBEDTLS_OID_AT "\x2B" /**< id-at-initials AttributeType:= {id-at 43} */
#define MBEDTLS_OID_AT_GENERATION_QUALIFIER     MBEDTLS_OID_AT "\x2C" /**< id-at-generationQualifier AttributeType:= {id-at 44} */
#define MBEDTLS_OID_AT_UNIQUE_IDENTIFIER        MBEDTLS_OID_AT "\x2D" /**< id-at-uniqueIdentifier AttributeType:= {id-at 45} */
#define MBEDTLS_OID_AT_DN_QUALIFIER             MBEDTLS_OID_AT "\x2E" /**< id-at-dnQualifier AttributeType:= {id-at 46} */
#define MBEDTLS_OID_AT_PSEUDONYM                MBEDTLS_OID_AT "\x41" /**< id-at-pseudonym AttributeType:= {id-at 65} */

#define MBEDTLS_OID_UID                         "\x09\x92\x26\x89\x93\xF2\x2C\x64\x01\x01" /** id-domainComponent AttributeType:= {itu-t(0) data(9) pss(2342) ucl(19200300) pilot(100) pilotAttributeType(1) uid(1)} */
#define MBEDTLS_OID_DOMAIN_COMPONENT            "\x09\x92\x26\x89\x93\xF2\x2C\x64\x01\x19" /** id-domainComponent AttributeType:= {itu-t(0) data(9) pss(2342) ucl(19200300) pilot(100) pilotAttributeType(1) domainComponent(25)} */

/*
 * OIDs for standard certificate extensions
 */
#define MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER    MBEDTLS_OID_ID_CE "\x23" /**< id-ce-authorityKeyIdentifier OBJECT IDENTIFIER ::=  { id-ce 35 } */
#define MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER      MBEDTLS_OID_ID_CE "\x0E" /**< id-ce-subjectKeyIdentifier OBJECT IDENTIFIER ::=  { id-ce 14 } */
#define MBEDTLS_OID_KEY_USAGE                   MBEDTLS_OID_ID_CE "\x0F" /**< id-ce-keyUsage OBJECT IDENTIFIER ::=  { id-ce 15 } */
#define MBEDTLS_OID_CERTIFICATE_POLICIES        MBEDTLS_OID_ID_CE "\x20" /**< id-ce-certificatePolicies OBJECT IDENTIFIER ::=  { id-ce 32 } */
#define MBEDTLS_OID_POLICY_MAPPINGS             MBEDTLS_OID_ID_CE "\x21" /**< id-ce-policyMappings OBJECT IDENTIFIER ::=  { id-ce 33 } */
#define MBEDTLS_OID_SUBJECT_ALT_NAME            MBEDTLS_OID_ID_CE "\x11" /**< id-ce-subjectAltName OBJECT IDENTIFIER ::=  { id-ce 17 } */
#define MBEDTLS_OID_ISSUER_ALT_NAME             MBEDTLS_OID_ID_CE "\x12" /**< id-ce-issuerAltName OBJECT IDENTIFIER ::=  { id-ce 18 } */
#define MBEDTLS_OID_SUBJECT_DIRECTORY_ATTRS     MBEDTLS_OID_ID_CE "\x09" /**< id-ce-subjectDirectoryAttributes OBJECT IDENTIFIER ::=  { id-ce 9 } */
#define MBEDTLS_OID_BASIC_CONSTRAINTS           MBEDTLS_OID_ID_CE "\x13" /**< id-ce-basicConstraints OBJECT IDENTIFIER ::=  { id-ce 19 } */
#define MBEDTLS_OID_NAME_CONSTRAINTS            MBEDTLS_OID_ID_CE "\x1E" /**< id-ce-nameConstraints OBJECT IDENTIFIER ::=  { id-ce 30 } */
#define MBEDTLS_OID_POLICY_CONSTRAINTS          MBEDTLS_OID_ID_CE "\x24" /**< id-ce-policyConstraints OBJECT IDENTIFIER ::=  { id-ce 36 } */
#define MBEDTLS_OID_EXTENDED_KEY_USAGE          MBEDTLS_OID_ID_CE "\x25" /**< id-ce-extKeyUsage OBJECT IDENTIFIER ::= { id-ce 37 } */
#define MBEDTLS_OID_CRL_DISTRIBUTION_POINTS     MBEDTLS_OID_ID_CE "\x1F" /**< id-ce-cRLDistributionPoints OBJECT IDENTIFIER ::=  { id-ce 31 } */
#define MBEDTLS_OID_INIHIBIT_ANYPOLICY          MBEDTLS_OID_ID_CE "\x36" /**< id-ce-inhibitAnyPolicy OBJECT IDENTIFIER ::=  { id-ce 54 } */
#define MBEDTLS_OID_FRESHEST_CRL                MBEDTLS_OID_ID_CE "\x2E" /**< id-ce-freshestCRL OBJECT IDENTIFIER ::=  { id-ce 46 } */

/*
 * Certificate policies
 */
#define MBEDTLS_OID_ANY_POLICY              MBEDTLS_OID_CERTIFICATE_POLICIES "\x00" /**< anyPolicy OBJECT IDENTIFIER ::= { id-ce-certificatePolicies 0 } */

/*
 * Netscape certificate extensions
 */
#define MBEDTLS_OID_NS_CERT                 MBEDTLS_OID_NETSCAPE "\x01"
#define MBEDTLS_OID_NS_CERT_TYPE            MBEDTLS_OID_NS_CERT  "\x01"
#define MBEDTLS_OID_NS_BASE_URL             MBEDTLS_OID_NS_CERT  "\x02"
#define MBEDTLS_OID_NS_REVOCATION_URL       MBEDTLS_OID_NS_CERT  "\x03"
#define MBEDTLS_OID_NS_CA_REVOCATION_URL    MBEDTLS_OID_NS_CERT  "\x04"
#define MBEDTLS_OID_NS_RENEWAL_URL          MBEDTLS_OID_NS_CERT  "\x07"
#define MBEDTLS_OID_NS_CA_POLICY_URL        MBEDTLS_OID_NS_CERT  "\x08"
#define MBEDTLS_OID_NS_SSL_SERVER_NAME      MBEDTLS_OID_NS_CERT  "\x0C"
#define MBEDTLS_OID_NS_COMMENT              MBEDTLS_OID_NS_CERT  "\x0D"
#define MBEDTLS_OID_NS_DATA_TYPE            MBEDTLS_OID_NETSCAPE "\x02"
#define MBEDTLS_OID_NS_CERT_SEQUENCE        MBEDTLS_OID_NS_DATA_TYPE "\x05"

/*
 * OIDs for CRL extensions
 */
#define MBEDTLS_OID_PRIVATE_KEY_USAGE_PERIOD    MBEDTLS_OID_ID_CE "\x10"
#define MBEDTLS_OID_CRL_NUMBER                  MBEDTLS_OID_ID_CE "\x14" /**< id-ce-cRLNumber OBJECT IDENTIFIER ::= { id-ce 20 } */

/*
 * X.509 v3 Extended key usage OIDs
 */
#define MBEDTLS_OID_ANY_EXTENDED_KEY_USAGE      MBEDTLS_OID_EXTENDED_KEY_USAGE "\x00" /**< anyExtendedKeyUsage OBJECT IDENTIFIER ::= { id-ce-extKeyUsage 0 } */

#define MBEDTLS_OID_KP                          MBEDTLS_OID_PKIX "\x03" /**< id-kp OBJECT IDENTIFIER ::= { id-pkix 3 } */
#define MBEDTLS_OID_SERVER_AUTH                 MBEDTLS_OID_KP "\x01" /**< id-kp-serverAuth OBJECT IDENTIFIER ::= { id-kp 1 } */
#define MBEDTLS_OID_CLIENT_AUTH                 MBEDTLS_OID_KP "\x02" /**< id-kp-clientAuth OBJECT IDENTIFIER ::= { id-kp 2 } */
#define MBEDTLS_OID_CODE_SIGNING                MBEDTLS_OID_KP "\x03" /**< id-kp-codeSigning OBJECT IDENTIFIER ::= { id-kp 3 } */
#define MBEDTLS_OID_EMAIL_PROTECTION            MBEDTLS_OID_KP "\x04" /**< id-kp-emailProtection OBJECT IDENTIFIER ::= { id-kp 4 } */
#define MBEDTLS_OID_TIME_STAMPING               MBEDTLS_OID_KP "\x08" /**< id-kp-timeStamping OBJECT IDENTIFIER ::= { id-kp 8 } */
#define MBEDTLS_OID_OCSP_SIGNING                MBEDTLS_OID_KP "\x09" /**< id-kp-OCSPSigning OBJECT IDENTIFIER ::= { id-kp 9 } */

/**
 * Wi-SUN Alliance Field Area Network
 * { iso(1) identified-organization(3) dod(6) internet(1)
 *                      private(4) enterprise(1) WiSUN(45605) FieldAreaNetwork(1) }
 */
#define MBEDTLS_OID_WISUN_FAN                   MBEDTLS_OID_INTERNET "\x04\x01\x82\xe4\x25\x01"

#define MBEDTLS_OID_ON                          MBEDTLS_OID_PKIX "\x08" /**< id-on OBJECT IDENTIFIER ::= { id-pkix 8 } */
#define MBEDTLS_OID_ON_HW_MODULE_NAME           MBEDTLS_OID_ON "\x04" /**< id-on-hardwareModuleName OBJECT IDENTIFIER ::= { id-on 4 } */

/*
 * PKCS definition OIDs
 */

#define MBEDTLS_OID_PKCS                MBEDTLS_OID_RSA_COMPANY "\x01" /**< pkcs OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) 1 } */
#define MBEDTLS_OID_PKCS1               MBEDTLS_OID_PKCS "\x01" /**< pkcs-1 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 1 } */
#define MBEDTLS_OID_PKCS5               MBEDTLS_OID_PKCS "\x05" /**< pkcs-5 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 5 } */
#define MBEDTLS_OID_PKCS7               MBEDTLS_OID_PKCS "\x07" /**< pkcs-7 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 7 } */
#define MBEDTLS_OID_PKCS9               MBEDTLS_OID_PKCS "\x09" /**< pkcs-9 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 9 } */
#define MBEDTLS_OID_PKCS12              MBEDTLS_OID_PKCS "\x0c" /**< pkcs-12 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 12 } */

/*
 * PKCS#1 OIDs
 */
#define MBEDTLS_OID_PKCS1_RSA           MBEDTLS_OID_PKCS1 "\x01" /**< rsaEncryption OBJECT IDENTIFIER ::= { pkcs-1 1 } */
#define MBEDTLS_OID_PKCS1_MD5           MBEDTLS_OID_PKCS1 "\x04" /**< md5WithRSAEncryption ::= { pkcs-1 4 } */
#define MBEDTLS_OID_PKCS1_SHA1          MBEDTLS_OID_PKCS1 "\x05" /**< sha1WithRSAEncryption ::= { pkcs-1 5 } */
#define MBEDTLS_OID_PKCS1_SHA224        MBEDTLS_OID_PKCS1 "\x0e" /**< sha224WithRSAEncryption ::= { pkcs-1 14 } */
#define MBEDTLS_OID_PKCS1_SHA256        MBEDTLS_OID_PKCS1 "\x0b" /**< sha256WithRSAEncryption ::= { pkcs-1 11 } */
#define MBEDTLS_OID_PKCS1_SHA384        MBEDTLS_OID_PKCS1 "\x0c" /**< sha384WithRSAEncryption ::= { pkcs-1 12 } */
#define MBEDTLS_OID_PKCS1_SHA512        MBEDTLS_OID_PKCS1 "\x0d" /**< sha512WithRSAEncryption ::= { pkcs-1 13 } */

#define MBEDTLS_OID_RSA_SHA_OBS         "\x2B\x0E\x03\x02\x1D"

#define MBEDTLS_OID_PKCS9_EMAIL         MBEDTLS_OID_PKCS9 "\x01" /**< emailAddress AttributeType ::= { pkcs-9 1 } */

/* RFC 4055 */
#define MBEDTLS_OID_RSASSA_PSS          MBEDTLS_OID_PKCS1 "\x0a" /**< id-RSASSA-PSS ::= { pkcs-1 10 } */
#define MBEDTLS_OID_MGF1                MBEDTLS_OID_PKCS1 "\x08" /**< id-mgf1 ::= { pkcs-1 8 } */

/*
 * Digest algorithms
 */
#define MBEDTLS_OID_DIGEST_ALG_MD5              MBEDTLS_OID_RSA_COMPANY "\x02\x05" /**< id-mbedtls_md5 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 5 } */
#define MBEDTLS_OID_DIGEST_ALG_SHA1             MBEDTLS_OID_ISO_IDENTIFIED_ORG \
        MBEDTLS_OID_OIW_SECSIG_SHA1                                                                        /**< id-mbedtls_sha1 OBJECT IDENTIFIER ::= { iso(1) identified-organization(3) oiw(14) secsig(3) algorithms(2) 26 } */
#define MBEDTLS_OID_DIGEST_ALG_SHA224           MBEDTLS_OID_NIST_ALG "\x02\x04" /**< id-sha224 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 4 } */
#define MBEDTLS_OID_DIGEST_ALG_SHA256           MBEDTLS_OID_NIST_ALG "\x02\x01" /**< id-mbedtls_sha256 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 1 } */

#define MBEDTLS_OID_DIGEST_ALG_SHA384           MBEDTLS_OID_NIST_ALG "\x02\x02" /**< id-sha384 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 2 } */

#define MBEDTLS_OID_DIGEST_ALG_SHA512           MBEDTLS_OID_NIST_ALG "\x02\x03" /**< id-mbedtls_sha512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 3 } */

#define MBEDTLS_OID_DIGEST_ALG_RIPEMD160        MBEDTLS_OID_TELETRUST "\x03\x02\x01" /**< id-ripemd160 OBJECT IDENTIFIER :: { iso(1) identified-organization(3) teletrust(36) algorithm(3) hashAlgorithm(2) ripemd160(1) } */

#define MBEDTLS_OID_DIGEST_ALG_SHA3_224         MBEDTLS_OID_NIST_ALG "\x02\x07" /**< id-sha3-224 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) sha3-224(7) } */

#define MBEDTLS_OID_DIGEST_ALG_SHA3_256         MBEDTLS_OID_NIST_ALG "\x02\x08" /**< id-sha3-256 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) sha3-256(8) } */

#define MBEDTLS_OID_DIGEST_ALG_SHA3_384         MBEDTLS_OID_NIST_ALG "\x02\x09" /**< id-sha3-384 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) sha3-384(9) } */

#define MBEDTLS_OID_DIGEST_ALG_SHA3_512         MBEDTLS_OID_NIST_ALG "\x02\x0a" /**< id-sha3-512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) sha3-512(10) } */

#define MBEDTLS_OID_HMAC_SHA1                   MBEDTLS_OID_RSA_COMPANY "\x02\x07" /**< id-hmacWithSHA1 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 7 } */

#define MBEDTLS_OID_HMAC_SHA224                 MBEDTLS_OID_RSA_COMPANY "\x02\x08" /**< id-hmacWithSHA224 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 8 } */

#define MBEDTLS_OID_HMAC_SHA256                 MBEDTLS_OID_RSA_COMPANY "\x02\x09" /**< id-hmacWithSHA256 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 9 } */

#define MBEDTLS_OID_HMAC_SHA384                 MBEDTLS_OID_RSA_COMPANY "\x02\x0A" /**< id-hmacWithSHA384 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 10 } */

#define MBEDTLS_OID_HMAC_SHA512                 MBEDTLS_OID_RSA_COMPANY "\x02\x0B" /**< id-hmacWithSHA512 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 11 } */

#define MBEDTLS_OID_HMAC_SHA3_224               MBEDTLS_OID_NIST_ALG "\x02\x0d" /**< id-hmacWithSHA3-512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) hmacWithSHA3-224(13) } */

#define MBEDTLS_OID_HMAC_SHA3_256               MBEDTLS_OID_NIST_ALG "\x02\x0e" /**< id-hmacWithSHA3-512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) hmacWithSHA3-256(14) } */

#define MBEDTLS_OID_HMAC_SHA3_384               MBEDTLS_OID_NIST_ALG "\x02\x0f" /**< id-hmacWithSHA3-512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) hmacWithSHA3-384(15) } */

#define MBEDTLS_OID_HMAC_SHA3_512               MBEDTLS_OID_NIST_ALG "\x02\x10" /**< id-hmacWithSHA3-512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) hashalgs(2) hmacWithSHA3-512(16) } */

#define MBEDTLS_OID_HMAC_RIPEMD160              MBEDTLS_OID_INTERNET "\x05\x05\x08\x01\x04" /**< id-hmacWithSHA1 OBJECT IDENTIFIER ::= {iso(1) iso-identified-organization(3) dod(6) internet(1) security(5) mechanisms(5) ipsec(8) isakmpOakley(1) hmacRIPEMD160(4)} */

/*
 * Encryption algorithms,
 * the following standardized object identifiers are specified at
 * https://datatracker.ietf.org/doc/html/rfc8018#appendix-C.
 */
#define MBEDTLS_OID_DES_CBC                     MBEDTLS_OID_ISO_IDENTIFIED_ORG \
        MBEDTLS_OID_OIW_SECSIG_ALG "\x07"                                                                        /**< desCBC OBJECT IDENTIFIER ::= { iso(1) identified-organization(3) oiw(14) secsig(3) algorithms(2) 7 } */
#define MBEDTLS_OID_DES_EDE3_CBC                MBEDTLS_OID_RSA_COMPANY "\x03\x07" /**< des-ede3-cbc OBJECT IDENTIFIER ::= { iso(1) member-body(2) -- us(840) rsadsi(113549) encryptionAlgorithm(3) 7 } */
#define MBEDTLS_OID_AES                         MBEDTLS_OID_NIST_ALG "\x01" /** aes OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithm(4) 1 } */
#define MBEDTLS_OID_AES_128_CBC                 MBEDTLS_OID_AES "\x02" /** aes128-cbc-pad OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) aes(1) aes128-CBC-PAD(2) } */
#define MBEDTLS_OID_AES_192_CBC                 MBEDTLS_OID_AES "\x16" /** aes192-cbc-pad OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) aes(1) aes192-CBC-PAD(22) } */
#define MBEDTLS_OID_AES_256_CBC                 MBEDTLS_OID_AES "\x2a" /** aes256-cbc-pad OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistAlgorithms(4) aes(1) aes256-CBC-PAD(42) } */

/*
 * Key Wrapping algorithms
 */
/*
 * RFC 5649
 */
#define MBEDTLS_OID_AES128_KW                   MBEDTLS_OID_AES "\x05" /** id-aes128-wrap     OBJECT IDENTIFIER ::= { aes 5 } */
#define MBEDTLS_OID_AES128_KWP                  MBEDTLS_OID_AES "\x08" /** id-aes128-wrap-pad OBJECT IDENTIFIER ::= { aes 8 } */
#define MBEDTLS_OID_AES192_KW                   MBEDTLS_OID_AES "\x19" /** id-aes192-wrap     OBJECT IDENTIFIER ::= { aes 25 } */
#define MBEDTLS_OID_AES192_KWP                  MBEDTLS_OID_AES "\x1c" /** id-aes192-wrap-pad OBJECT IDENTIFIER ::= { aes 28 } */
#define MBEDTLS_OID_AES256_KW                   MBEDTLS_OID_AES "\x2d" /** id-aes256-wrap     OBJECT IDENTIFIER ::= { aes 45 } */
#define MBEDTLS_OID_AES256_KWP                  MBEDTLS_OID_AES "\x30" /** id-aes256-wrap-pad OBJECT IDENTIFIER ::= { aes 48 } */
/*
 * PKCS#5 OIDs
 */
#define MBEDTLS_OID_PKCS5_PBKDF2                MBEDTLS_OID_PKCS5 "\x0c" /**< id-PBKDF2 OBJECT IDENTIFIER ::= {pkcs-5 12} */
#define MBEDTLS_OID_PKCS5_PBES2                 MBEDTLS_OID_PKCS5 "\x0d" /**< id-PBES2 OBJECT IDENTIFIER ::= {pkcs-5 13} */
#define MBEDTLS_OID_PKCS5_PBMAC1                MBEDTLS_OID_PKCS5 "\x0e" /**< id-PBMAC1 OBJECT IDENTIFIER ::= {pkcs-5 14} */

/*
 * PKCS#5 PBES1 algorithms
 */
#define MBEDTLS_OID_PKCS5_PBE_MD5_DES_CBC       MBEDTLS_OID_PKCS5 "\x03" /**< pbeWithMD5AndDES-CBC OBJECT IDENTIFIER ::= {pkcs-5 3} */
#define MBEDTLS_OID_PKCS5_PBE_MD5_RC2_CBC       MBEDTLS_OID_PKCS5 "\x06" /**< pbeWithMD5AndRC2-CBC OBJECT IDENTIFIER ::= {pkcs-5 6} */
#define MBEDTLS_OID_PKCS5_PBE_SHA1_DES_CBC      MBEDTLS_OID_PKCS5 "\x0a" /**< pbeWithSHA1AndDES-CBC OBJECT IDENTIFIER ::= {pkcs-5 10} */
#define MBEDTLS_OID_PKCS5_PBE_SHA1_RC2_CBC      MBEDTLS_OID_PKCS5 "\x0b" /**< pbeWithSHA1AndRC2-CBC OBJECT IDENTIFIER ::= {pkcs-5 11} */

/*
 * PKCS#7 OIDs
 */
#define MBEDTLS_OID_PKCS7_DATA                        MBEDTLS_OID_PKCS7 "\x01" /**< Content type is Data OBJECT IDENTIFIER ::= {pkcs-7 1} */
#define MBEDTLS_OID_PKCS7_SIGNED_DATA                 MBEDTLS_OID_PKCS7 "\x02" /**< Content type is Signed Data OBJECT IDENTIFIER ::= {pkcs-7 2} */
#define MBEDTLS_OID_PKCS7_ENVELOPED_DATA              MBEDTLS_OID_PKCS7 "\x03" /**< Content type is Enveloped Data OBJECT IDENTIFIER ::= {pkcs-7 3} */
#define MBEDTLS_OID_PKCS7_SIGNED_AND_ENVELOPED_DATA   MBEDTLS_OID_PKCS7 "\x04" /**< Content type is Signed and Enveloped Data OBJECT IDENTIFIER ::= {pkcs-7 4} */
#define MBEDTLS_OID_PKCS7_DIGESTED_DATA               MBEDTLS_OID_PKCS7 "\x05" /**< Content type is Digested Data OBJECT IDENTIFIER ::= {pkcs-7 5} */
#define MBEDTLS_OID_PKCS7_ENCRYPTED_DATA              MBEDTLS_OID_PKCS7 "\x06" /**< Content type is Encrypted Data OBJECT IDENTIFIER ::= {pkcs-7 6} */

/*
 * PKCS#8 OIDs
 */
#define MBEDTLS_OID_PKCS9_CSR_EXT_REQ           MBEDTLS_OID_PKCS9 "\x0e" /**< extensionRequest OBJECT IDENTIFIER ::= {pkcs-9 14} */

/*
 * PKCS#12 PBE OIDs
 */
#define MBEDTLS_OID_PKCS12_PBE                      MBEDTLS_OID_PKCS12 "\x01" /**< pkcs-12PbeIds OBJECT IDENTIFIER ::= {pkcs-12 1} */

#define MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC    MBEDTLS_OID_PKCS12_PBE "\x03" /**< pbeWithSHAAnd3-KeyTripleDES-CBC OBJECT IDENTIFIER ::= {pkcs-12PbeIds 3} */
#define MBEDTLS_OID_PKCS12_PBE_SHA1_DES2_EDE_CBC    MBEDTLS_OID_PKCS12_PBE "\x04" /**< pbeWithSHAAnd2-KeyTripleDES-CBC OBJECT IDENTIFIER ::= {pkcs-12PbeIds 4} */
#define MBEDTLS_OID_PKCS12_PBE_SHA1_RC2_128_CBC     MBEDTLS_OID_PKCS12_PBE "\x05" /**< pbeWithSHAAnd128BitRC2-CBC OBJECT IDENTIFIER ::= {pkcs-12PbeIds 5} */
#define MBEDTLS_OID_PKCS12_PBE_SHA1_RC2_40_CBC      MBEDTLS_OID_PKCS12_PBE "\x06" /**< pbeWithSHAAnd40BitRC2-CBC OBJECT IDENTIFIER ::= {pkcs-12PbeIds 6} */

/*
 * EC key algorithms from RFC 5480
 */

/* id-ecPublicKey OBJECT IDENTIFIER ::= {
 *       iso(1) member-body(2) us(840) ansi-X9-62(10045) keyType(2) 1 } */
#define MBEDTLS_OID_EC_ALG_UNRESTRICTED         MBEDTLS_OID_ANSI_X9_62 "\x02\01"

/*   id-ecDH OBJECT IDENTIFIER ::= {
 *     iso(1) identified-organization(3) certicom(132)
 *     schemes(1) ecdh(12) } */
#define MBEDTLS_OID_EC_ALG_ECDH                 MBEDTLS_OID_CERTICOM "\x01\x0c"

/*
 * ECParameters namedCurve identifiers, from RFC 5480, RFC 5639, and SEC2
 */

/* secp192r1 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) curves(3) prime(1) 1 } */
#define MBEDTLS_OID_EC_GRP_SECP192R1        MBEDTLS_OID_ANSI_X9_62 "\x03\x01\x01"

/* secp224r1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 33 } */
#define MBEDTLS_OID_EC_GRP_SECP224R1        MBEDTLS_OID_CERTICOM "\x00\x21"

/* secp256r1 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) curves(3) prime(1) 7 } */
#define MBEDTLS_OID_EC_GRP_SECP256R1        MBEDTLS_OID_ANSI_X9_62 "\x03\x01\x07"

/* secp384r1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 34 } */
#define MBEDTLS_OID_EC_GRP_SECP384R1        MBEDTLS_OID_CERTICOM "\x00\x22"

/* secp521r1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 35 } */
#define MBEDTLS_OID_EC_GRP_SECP521R1        MBEDTLS_OID_CERTICOM "\x00\x23"

/* secp192k1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 31 } */
#define MBEDTLS_OID_EC_GRP_SECP192K1        MBEDTLS_OID_CERTICOM "\x00\x1f"

/* secp224k1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 32 } */
#define MBEDTLS_OID_EC_GRP_SECP224K1        MBEDTLS_OID_CERTICOM "\x00\x20"

/* secp256k1 OBJECT IDENTIFIER ::= {
 *   iso(1) identified-organization(3) certicom(132) curve(0) 10 } */
#define MBEDTLS_OID_EC_GRP_SECP256K1        MBEDTLS_OID_CERTICOM "\x00\x0a"

/* RFC 5639 4.1
 * ecStdCurvesAndGeneration OBJECT IDENTIFIER::= {iso(1)
 * identified-organization(3) teletrust(36) algorithm(3) signature-
 * algorithm(3) ecSign(2) 8}
 * ellipticCurve OBJECT IDENTIFIER ::= {ecStdCurvesAndGeneration 1}
 * versionOne OBJECT IDENTIFIER ::= {ellipticCurve 1} */
#define MBEDTLS_OID_EC_BRAINPOOL_V1         MBEDTLS_OID_TELETRUST "\x03\x03\x02\x08\x01\x01"

/* brainpoolP256r1 OBJECT IDENTIFIER ::= {versionOne 7} */
#define MBEDTLS_OID_EC_GRP_BP256R1          MBEDTLS_OID_EC_BRAINPOOL_V1 "\x07"

/* brainpoolP384r1 OBJECT IDENTIFIER ::= {versionOne 11} */
#define MBEDTLS_OID_EC_GRP_BP384R1          MBEDTLS_OID_EC_BRAINPOOL_V1 "\x0B"

/* brainpoolP512r1 OBJECT IDENTIFIER ::= {versionOne 13} */
#define MBEDTLS_OID_EC_GRP_BP512R1          MBEDTLS_OID_EC_BRAINPOOL_V1 "\x0D"

/*
 * SEC1 C.1
 *
 * prime-field OBJECT IDENTIFIER ::= { id-fieldType 1 }
 * id-fieldType OBJECT IDENTIFIER ::= { ansi-X9-62 fieldType(1)}
 */
#define MBEDTLS_OID_ANSI_X9_62_FIELD_TYPE   MBEDTLS_OID_ANSI_X9_62 "\x01"
#define MBEDTLS_OID_ANSI_X9_62_PRIME_FIELD  MBEDTLS_OID_ANSI_X9_62_FIELD_TYPE "\x01"

/*
 * ECDSA signature identifiers, from RFC 5480
 */
#define MBEDTLS_OID_ANSI_X9_62_SIG          MBEDTLS_OID_ANSI_X9_62 "\x04" /* signatures(4) */
#define MBEDTLS_OID_ANSI_X9_62_SIG_SHA2     MBEDTLS_OID_ANSI_X9_62_SIG "\x03" /* ecdsa-with-SHA2(3) */

/* ecdsa-with-SHA1 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) signatures(4) 1 } */
#define MBEDTLS_OID_ECDSA_SHA1              MBEDTLS_OID_ANSI_X9_62_SIG "\x01"

/* ecdsa-with-SHA224 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) signatures(4)
 *   ecdsa-with-SHA2(3) 1 } */
#define MBEDTLS_OID_ECDSA_SHA224            MBEDTLS_OID_ANSI_X9_62_SIG_SHA2 "\x01"

/* ecdsa-with-SHA256 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) signatures(4)
 *   ecdsa-with-SHA2(3) 2 } */
#define MBEDTLS_OID_ECDSA_SHA256            MBEDTLS_OID_ANSI_X9_62_SIG_SHA2 "\x02"

/* ecdsa-with-SHA384 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) signatures(4)
 *   ecdsa-with-SHA2(3) 3 } */
#define MBEDTLS_OID_ECDSA_SHA384            MBEDTLS_OID_ANSI_X9_62_SIG_SHA2 "\x03"

/* ecdsa-with-SHA512 OBJECT IDENTIFIER ::= {
 *   iso(1) member-body(2) us(840) ansi-X9-62(10045) signatures(4)
 *   ecdsa-with-SHA2(3) 4 } */
#define MBEDTLS_OID_ECDSA_SHA512            MBEDTLS_OID_ANSI_X9_62_SIG_SHA2 "\x04"

/*
 * EC key algorithms from RFC 8410
 */

#define MBEDTLS_OID_X25519                  MBEDTLS_OID_THAWTE "\x6e" /**< id-X25519    OBJECT IDENTIFIER ::= { 1 3 101 110 } */
#define MBEDTLS_OID_X448                    MBEDTLS_OID_THAWTE "\x6f" /**< id-X448      OBJECT IDENTIFIER ::= { 1 3 101 111 } */
#define MBEDTLS_OID_ED25519                 MBEDTLS_OID_THAWTE "\x70" /**< id-Ed25519   OBJECT IDENTIFIER ::= { 1 3 101 112 } */
#define MBEDTLS_OID_ED448                   MBEDTLS_OID_THAWTE "\x71" /**< id-Ed448     OBJECT IDENTIFIER ::= { 1 3 101 113 } */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Base OID descriptor structure
 */
typedef struct mbedtls_oid_descriptor_t {
    const char *MBEDTLS_PRIVATE(asn1);               /*!< OID ASN.1 representation       */
    size_t MBEDTLS_PRIVATE(asn1_len);                /*!< length of asn1                 */
#if !defined(MBEDTLS_X509_REMOVE_INFO)
    const char *MBEDTLS_PRIVATE(name);               /*!< official name (e.g. from RFC)  */
    const char *MBEDTLS_PRIVATE(description);        /*!< human friendly description     */
#endif
} mbedtls_oid_descriptor_t;

/**
 * \brief          Translate an X.509 extension OID into local values
 *
 * \param oid      OID to use
 * \param ext_type place to store the extension type
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_x509_ext_type(const mbedtls_asn1_buf *oid, int *ext_type, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate an X.509 attribute type OID into the short name
 *                 (e.g. the OID for an X520 Common Name into "CN")
 *
 * \param oid      OID to use
 * \param short_name    place to store the string pointer
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_attr_short_name(const mbedtls_asn1_buf *oid, const char **short_name, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate PublicKeyAlgorithm OID into pk_type
 *
 * \param oid      OID to use
 * \param pk_alg   place to store public key algorithm
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_pk_alg(const mbedtls_asn1_buf *oid, mbedtls_pk_type_t *pk_alg, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate pk_type into PublicKeyAlgorithm OID
 *
 * \param pk_alg   Public key type to look for
 * \param oid      place to store ASN.1 OID string pointer
 * \param olen     length of the OID
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_oid_by_pk_alg(mbedtls_pk_type_t pk_alg,
                                  const char **oid, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate NamedCurve OID into an EC group identifier
 *
 * \param oid      OID to use
 * \param grp_id   place to store group id
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_ec_grp(const mbedtls_asn1_buf *oid, mbedtls_ecp_group_id *grp_id, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate EC group identifier into NamedCurve OID
 *
 * \param grp_id   EC group identifier
 * \param oid      place to store ASN.1 OID string pointer
 * \param olen     length of the OID
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_oid_by_ec_grp(mbedtls_ecp_group_id grp_id,
                                  const char **oid, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate AlgorithmIdentifier OID into an EC group identifier,
 *                 for curves that are directly encoded at this level
 *
 * \param oid      OID to use
 * \param grp_id   place to store group id
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_ec_grp_algid(const mbedtls_asn1_buf *oid, mbedtls_ecp_group_id *grp_id, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate EC group identifier into AlgorithmIdentifier OID,
 *                 for curves that are directly encoded at this level
 *
 * \param grp_id   EC group identifier
 * \param oid      place to store ASN.1 OID string pointer
 * \param olen     length of the OID
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_oid_by_ec_grp_algid(mbedtls_ecp_group_id grp_id,
                                        const char **oid, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate SignatureAlgorithm OID into md_type and pk_type
 *
 * \param oid      OID to use
 * \param md_alg   place to store message digest algorithm
 * \param pk_alg   place to store public key algorithm
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_sig_alg(const mbedtls_asn1_buf *oid,
                            mbedtls_md_type_t *md_alg, mbedtls_pk_type_t *pk_alg, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate SignatureAlgorithm OID into description
 *
 * \param oid      OID to use
 * \param desc     place to store string pointer
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_sig_alg_desc(const mbedtls_asn1_buf *oid, const char **desc, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate md_type and pk_type into SignatureAlgorithm OID
 *
 * \param md_alg   message digest algorithm
 * \param pk_alg   public key algorithm
 * \param oid      place to store ASN.1 OID string pointer
 * \param olen     length of the OID
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_oid_by_sig_alg(mbedtls_pk_type_t pk_alg, mbedtls_md_type_t md_alg,
                                   const char **oid, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate hmac algorithm OID into md_type
 *
 * \param oid      OID to use
 * \param md_hmac  place to store message hmac algorithm
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_md_hmac(const mbedtls_asn1_buf *oid, mbedtls_md_type_t *md_hmac, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate hash algorithm OID into md_type
 *
 * \param oid      OID to use
 * \param md_alg   place to store message digest algorithm
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_md_alg(const mbedtls_asn1_buf *oid, mbedtls_md_type_t *md_alg, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_X509_REMOVE_INFO)
/**
 * \brief          Translate Extended Key Usage OID into description
 *
 * \param oid      OID to use
 * \param desc     place to store string pointer
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_extended_key_usage(const mbedtls_asn1_buf *oid, const char **desc, MinTlsDiagnostics* diagnostics);
#endif

/**
 * \brief          Translate certificate policies OID into description
 *
 * \param oid      OID to use
 * \param desc     place to store string pointer
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_certificate_policies(const mbedtls_asn1_buf *oid, const char **desc, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate md_type into hash algorithm OID
 *
 * \param md_alg   message digest algorithm
 * \param oid      place to store ASN.1 OID string pointer
 * \param olen     length of the OID
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_oid_by_md(mbedtls_md_type_t md_alg, const char **oid, size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Translate encryption algorithm OID into cipher_type
 *
 * \param oid           OID to use
 * \param cipher_alg    place to store cipher algorithm
 *
 * \return         0 if successful, or MBEDTLS_ERR_OID_NOT_FOUND
 */
int mbedtls_oid_get_cipher_alg(const mbedtls_asn1_buf *oid, mbedtls_cipher_type_t *cipher_alg, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* oid.h */

/* ===== Retained pem.h ===== */

/**
 * \file pem.h
 *
 * \brief Privacy Enhanced Mail (PEM) decoding
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/pem.h.
 */
#ifndef MBEDTLS_PEM_H
#define MBEDTLS_PEM_H


#include <stddef.h>

/**
 * \name PEM Error codes
 * These error codes are returned in case of errors reading the
 * PEM data.
 * \{
 */
/** No PEM header or footer found. */
#define MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT          -0x1080
/** PEM string is not as expected. */
#define MBEDTLS_ERR_PEM_INVALID_DATA                      -0x1100
/** Failed to allocate memory. */
#define MBEDTLS_ERR_PEM_ALLOC_FAILED                      -0x1180
/** RSA IV is not in hex-format. */
#define MBEDTLS_ERR_PEM_INVALID_ENC_IV                    -0x1200
/** Unsupported key encryption algorithm. */
#define MBEDTLS_ERR_PEM_UNKNOWN_ENC_ALG                   -0x1280
/** Private key password can't be empty. */
#define MBEDTLS_ERR_PEM_PASSWORD_REQUIRED                 -0x1300
/** Given private key password does not allow for correct decryption. */
#define MBEDTLS_ERR_PEM_PASSWORD_MISMATCH                 -0x1380
/** Unavailable feature, e.g. hashing/encryption combination. */
#define MBEDTLS_ERR_PEM_FEATURE_UNAVAILABLE               -0x1400
/** Bad input parameters to function. */
#define MBEDTLS_ERR_PEM_BAD_INPUT_DATA                    -0x1480
/** \} name PEM Error codes */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief       PEM context structure
 */
typedef struct mbedtls_pem_context {
    unsigned char *MBEDTLS_PRIVATE(buf);     /*!< buffer for decoded data             */
    size_t MBEDTLS_PRIVATE(buflen);          /*!< length of the buffer                */
    unsigned char *MBEDTLS_PRIVATE(info);    /*!< buffer for extra header information */
}
mbedtls_pem_context;

/**
 * \brief       PEM context setup
 *
 * \param ctx   context to be initialized
 */
void mbedtls_pem_init(mbedtls_pem_context *ctx);

/**
 * \brief       Read a buffer for PEM information and store the resulting
 *              data into the specified context buffers.
 *
 * \param ctx       context to use
 * \param header    header string to seek and expect
 * \param footer    footer string to seek and expect
 * \param data      source data to look in (must be nul-terminated)
 * \param pwd       password for decryption (can be NULL)
 * \param pwdlen    length of password
 * \param use_len   destination for total length used from data buffer. It is
 *                  set after header is correctly read, so unless you get
 *                  MBEDTLS_ERR_PEM_BAD_INPUT_DATA or
 *                  MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT, use_len is
 *                  the length to skip.
 *
 * \note            Attempts to check password correctness by verifying if
 *                  the decrypted text starts with an ASN.1 sequence of
 *                  appropriate length
 *
 * \note            \c mbedtls_pem_free must be called on PEM context before
 *                  the PEM context can be reused in another call to
 *                  \c mbedtls_pem_read_buffer
 *
 * \return          0 on success, or a specific PEM error code
 */
int mbedtls_pem_read_buffer(mbedtls_pem_context *ctx, const char *header, const char *footer,
                            const unsigned char *data,
                            const unsigned char *pwd,
                            size_t pwdlen, size_t *use_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief       PEM context memory freeing
 *
 * \param ctx   context to be freed
 */
void mbedtls_pem_free(mbedtls_pem_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* pem.h */

/* ===== Retained pk_internal.h ===== */

/**
 * \file pk_internal.h
 *
 * \brief Public Key abstraction layer: internal (i.e. library only) functions
 *        and definitions.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pk_internal.h.
 */
#ifndef MBEDTLS_PK_INTERNAL_H
#define MBEDTLS_PK_INTERNAL_H



/* Headers/footers for PEM files */
#define PEM_BEGIN_PUBLIC_KEY    "-----BEGIN PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY      "-----END PUBLIC KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_RSA   "-----BEGIN RSA PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_RSA     "-----END RSA PRIVATE KEY-----"
#define PEM_BEGIN_PUBLIC_KEY_RSA     "-----BEGIN RSA PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY_RSA     "-----END RSA PUBLIC KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_EC    "-----BEGIN EC PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_EC      "-----END EC PRIVATE KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_PKCS8 "-----BEGIN PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_PKCS8   "-----END PRIVATE KEY-----"
#define PEM_BEGIN_ENCRYPTED_PRIVATE_KEY_PKCS8 "-----BEGIN ENCRYPTED PRIVATE KEY-----"
#define PEM_END_ENCRYPTED_PRIVATE_KEY_PKCS8   "-----END ENCRYPTED PRIVATE KEY-----"

/*
 * We're trying to statisfy two kinds of users:
 * - those who don't want to use the heap;
 * - those who can't afford large stack buffers.
 *
 * The current compromise is that if ECC is the only key type supported in PK,
 * then we export keys on the stack, and otherwise we use the heap.
 *
 * RSA can either be used directly or indirectly via opaque keys if enabled.
 * (RSA_ALT is not relevant here as we can't export from such contexts.)
 */

#if defined(PK_EXPORT_KEYS_ON_THE_STACK)
/* We know for ECC, pubkey are longer than privkeys, but double check.
 * Also, take the maximum size of legacy and PSA, as PSA might be disabled. */
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_PSA_MAX_EC_PUBKEY_LENGTH
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH
#endif
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_ECP_MAX_BYTES
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_ECP_MAX_BYTES
#endif
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_ECP_MAX_PT_LEN
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_ECP_MAX_PT_LEN
#endif
#endif

/**
 * Public function mbedtls_pk_ec() can be used to get direct access to the
 * wrapped ecp_keypair structure pointed to the pk_ctx. However this is not
 * ideal because it bypasses the PK module on the control of its internal
 * structure (pk_context) fields.
 * For backward compatibility we keep mbedtls_pk_ec() when ECP_C is defined, but
 * we provide 2 very similar functions when only ECP_LIGHT is enabled and not
 * ECP_C.
 * These variants embed the "ro" or "rw" keywords in their name to make the
 * usage of the returned pointer explicit. Of course the returned value is
 * const or non-const accordingly.
 */
static inline const mbedtls_ecp_keypair *mbedtls_pk_ec_ro(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
        case MBEDTLS_PK_ECDSA:
            return (const mbedtls_ecp_keypair *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

static inline mbedtls_ecp_keypair *mbedtls_pk_ec_rw(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
        case MBEDTLS_PK_ECDSA:
            return (mbedtls_ecp_keypair *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

static inline mbedtls_ecp_group_id mbedtls_pk_get_ec_group_id(const mbedtls_pk_context *pk)
{
    mbedtls_ecp_group_id id;

    {
        id = mbedtls_pk_ec_ro(*pk)->grp.id;
    }

    return id;
}

/* Helper for Montgomery curves */

#define MBEDTLS_PK_IS_RFC8410_GROUP_ID(id)  \
    ((id == MBEDTLS_ECP_DP_CURVE25519) || (id == MBEDTLS_ECP_DP_CURVE448))

/*
 * Set the group used by this key.
 *
 * [in/out] pk: in: must have been pk_setup() to an ECC type
 *              out: will have group (curve) information set
 * [in] grp_in: a supported group ID (not NONE)
 */
int mbedtls_pk_ecc_set_group(mbedtls_pk_context *pk, mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics);

/*
 * Set the public key.
 *
 * [in/out] pk: in: must have its group set, see mbedtls_pk_ecc_set_group().
 *              out: will have the public key set.
 * [in] pub, pub_len: the raw public key (an ECPoint).
 *
 * Return:
 * - 0 on success;
 * - MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if the format is potentially valid
 *   but not supported;
 * - another error code otherwise.
 */
int mbedtls_pk_ecc_set_pubkey(mbedtls_pk_context *pk, const unsigned char *pub, size_t pub_len, MinTlsDiagnostics* diagnostics);

/* Helper for (deterministic) ECDSA */
#define MBEDTLS_PK_PSA_ALG_ECDSA_MAYBE_DET  PSA_ALG_ECDSA

#endif /* MBEDTLS_PK_INTERNAL_H */

/* ===== Retained pk_wrap.h ===== */

/**
 * \file pk_wrap.h
 *
 * \brief Public Key abstraction layer: wrapper functions
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pk_wrap.h.
 */

#ifndef MBEDTLS_PK_WRAP_H
#define MBEDTLS_PK_WRAP_H



struct mbedtls_pk_info_t {
    /** Public key type */
    mbedtls_pk_type_t type;

    /** Type name */
    const char *name;

    /** Get key size in bits */
    size_t (*get_bitlen)(mbedtls_pk_context *pk);

    /** Tell if the context implements this type (e.g. ECKEY can do ECDSA) */
    int (*can_do)(mbedtls_pk_type_t type);

    /** Verify signature */
    int (*verify_func)(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                       const unsigned char *hash, size_t hash_len,
                       const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics);

    /** Make signature */
    int (*sign_func)(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                     const unsigned char *hash, size_t hash_len,
                     unsigned char *sig, size_t sig_size, size_t *sig_len,
                     int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                     void *p_rng, MinTlsDiagnostics* diagnostics);

    /** Decrypt message */
    int (*decrypt_func)(mbedtls_pk_context *pk, const unsigned char *input, size_t ilen,
                        unsigned char *output, size_t *olen, size_t osize,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng, MinTlsDiagnostics* diagnostics);

    /** Encrypt message */
    int (*encrypt_func)(mbedtls_pk_context *pk, const unsigned char *input, size_t ilen,
                        unsigned char *output, size_t *olen, size_t osize,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng, MinTlsDiagnostics* diagnostics);

    /** Check public-private key pair */
    int (*check_pair_func)(mbedtls_pk_context *pub, mbedtls_pk_context *prv,
                           int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                           void *p_rng, MinTlsDiagnostics* diagnostics);

    /** Allocate a new context */
    void * (*ctx_alloc_func)(void);

    /** Free the given context */
    void (*ctx_free_func)(void *ctx);

    /** Interface with the debug module */
    void (*debug_func)(mbedtls_pk_context *pk, mbedtls_pk_debug_item *items);

};
#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/* Container for RSA-alt */
typedef struct {
    void *key;
    mbedtls_pk_rsa_alt_decrypt_func decrypt_func;
    mbedtls_pk_rsa_alt_sign_func sign_func;
    mbedtls_pk_rsa_alt_key_len_func key_len_func;
} mbedtls_rsa_alt_context;
#endif

extern const mbedtls_pk_info_t mbedtls_rsa_info;

extern const mbedtls_pk_info_t mbedtls_eckey_info;
extern const mbedtls_pk_info_t mbedtls_eckeydh_info;

extern const mbedtls_pk_info_t mbedtls_ecdsa_info;

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
extern const mbedtls_pk_info_t mbedtls_rsa_alt_info;
#endif

#endif /* MBEDTLS_PK_WRAP_H */

/* ===== Retained platform.h ===== */

/**
 * \file platform.h
 *
 * \brief This file contains the definitions and functions of the
 *        Mbed TLS platform abstraction layer.
 *
 *        The platform abstraction layer removes the need for the library
 *        to directly link to standard C library functions or operating
 *        system services, making the library easier to port and embed.
 *        Application developers and users of the library can provide their own
 *        implementations of these functions, or implementations specific to
 *        their platform, which can be statically linked to the library or
 *        dynamically configured at runtime.
 *
 *        When all compilation options related to platform abstraction are
 *        disabled, this header just defines `mbedtls_xxx` function names
 *        as aliases to the standard `xxx` function.
 *
 *        Most modules in the library and example programs are expected to
 *        include this header.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/platform.h.
 */
#ifndef MBEDTLS_PLATFORM_H
#define MBEDTLS_PLATFORM_H



#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h or define them on the compiler command line.
 * \{
 */

/* The older Microsoft Windows common runtime provides non-conforming
 * implementations of some standard library functions, including snprintf
 * and vsnprintf. This affects MSVC and MinGW builds.
 */
#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER <= 1900)
#define MBEDTLS_PLATFORM_HAS_NON_CONFORMING_SNPRINTF
#define MBEDTLS_PLATFORM_HAS_NON_CONFORMING_VSNPRINTF
#endif

#if !defined(MBEDTLS_PLATFORM_NO_STD_FUNCTIONS)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined(MBEDTLS_PLATFORM_STD_SNPRINTF)
#if defined(MBEDTLS_PLATFORM_HAS_NON_CONFORMING_SNPRINTF)
#define MBEDTLS_PLATFORM_STD_SNPRINTF   mbedtls_platform_win32_snprintf /**< The default \c snprintf function to use.  */
#else
#define MBEDTLS_PLATFORM_STD_SNPRINTF   snprintf /**< The default \c snprintf function to use.  */
#endif
#endif
#if !defined(MBEDTLS_PLATFORM_STD_VSNPRINTF)
#if defined(MBEDTLS_PLATFORM_HAS_NON_CONFORMING_VSNPRINTF)
#define MBEDTLS_PLATFORM_STD_VSNPRINTF   mbedtls_platform_win32_vsnprintf /**< The default \c vsnprintf function to use.  */
#else
#define MBEDTLS_PLATFORM_STD_VSNPRINTF   vsnprintf /**< The default \c vsnprintf function to use.  */
#endif
#endif
#if !defined(MBEDTLS_PLATFORM_STD_PRINTF)
#define MBEDTLS_PLATFORM_STD_PRINTF   printf /**< The default \c printf function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_FPRINTF)
#define MBEDTLS_PLATFORM_STD_FPRINTF fprintf /**< The default \c fprintf function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_CALLOC)
#define MBEDTLS_PLATFORM_STD_CALLOC   calloc /**< The default \c calloc function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_FREE)
#define MBEDTLS_PLATFORM_STD_FREE       free /**< The default \c free function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_SETBUF)
#define MBEDTLS_PLATFORM_STD_SETBUF   setbuf /**< The default \c setbuf function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_EXIT)
#define MBEDTLS_PLATFORM_STD_EXIT      exit /**< The default \c exit function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_TIME)
#define MBEDTLS_PLATFORM_STD_TIME       time    /**< The default \c time function to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_EXIT_SUCCESS)
#define MBEDTLS_PLATFORM_STD_EXIT_SUCCESS  EXIT_SUCCESS /**< The default exit value to use. */
#endif
#if !defined(MBEDTLS_PLATFORM_STD_EXIT_FAILURE)
#define MBEDTLS_PLATFORM_STD_EXIT_FAILURE  EXIT_FAILURE /**< The default exit value to use. */
#endif
#else
#if defined(MBEDTLS_PLATFORM_STD_MEM_HDR)
#include MBEDTLS_PLATFORM_STD_MEM_HDR
#endif
#endif /* MBEDTLS_PLATFORM_NO_STD_FUNCTIONS */

/* Enable certain documented defines only when generating doxygen to avoid
 * an "unrecognized define" error. */
#if defined(__DOXYGEN__) && !defined(MBEDTLS_PLATFORM_STD_CALLOC)
#define MBEDTLS_PLATFORM_STD_CALLOC
#endif

#if defined(__DOXYGEN__) && !defined(MBEDTLS_PLATFORM_STD_FREE)
#define MBEDTLS_PLATFORM_STD_FREE
#endif

/** \} name SECTION: Module settings */

/*
 * The function pointers for calloc and free.
 * Please see MBEDTLS_PLATFORM_STD_CALLOC and MBEDTLS_PLATFORM_STD_FREE
 * in mbedtls_config.h for more information about behaviour and requirements.
 */
#if defined(MBEDTLS_PLATFORM_MEMORY)
#if defined(MBEDTLS_PLATFORM_FREE_MACRO) && \
    defined(MBEDTLS_PLATFORM_CALLOC_MACRO)
#undef mbedtls_free
#undef mbedtls_calloc
#define mbedtls_free       MBEDTLS_PLATFORM_FREE_MACRO
#define mbedtls_calloc     MBEDTLS_PLATFORM_CALLOC_MACRO
#else
/* For size_t */
#include <stddef.h>
extern void *mbedtls_calloc(size_t n, size_t size);
extern void mbedtls_free(void *ptr);

/**
 * \brief               This function dynamically sets the memory-management
 *                      functions used by the library, during runtime.
 *
 * \param calloc_func   The \c calloc function implementation.
 * \param free_func     The \c free function implementation.
 *
 * \return              \c 0.
 */
int mbedtls_platform_set_calloc_free(void *(*calloc_func)(size_t, size_t),
                                     void (*free_func)(void *));
#endif /* MBEDTLS_PLATFORM_FREE_MACRO && MBEDTLS_PLATFORM_CALLOC_MACRO */
#else
#undef mbedtls_free
#undef mbedtls_calloc
#define mbedtls_free       free
#define mbedtls_calloc     calloc
#endif /* MBEDTLS_PLATFORM_MEMORY && !MBEDTLS_PLATFORM_{FREE,CALLOC}_MACRO */

/*
 * The function pointers for fprintf
 */
#if defined(MBEDTLS_PLATFORM_FPRINTF_ALT)
/* We need FILE * */
#include <stdio.h>
extern int (*mbedtls_fprintf)(FILE *stream, const char *format, ...);

/**
 * \brief                This function dynamically configures the fprintf
 *                       function that is called when the
 *                       mbedtls_fprintf() function is invoked by the library.
 *
 * \param fprintf_func   The \c fprintf function implementation.
 *
 * \return               \c 0.
 */
int mbedtls_platform_set_fprintf(int (*fprintf_func)(FILE *stream, const char *,
                                                     ...));
#else
#undef mbedtls_fprintf
#if defined(MBEDTLS_PLATFORM_FPRINTF_MACRO)
#define mbedtls_fprintf    MBEDTLS_PLATFORM_FPRINTF_MACRO
#else
#define mbedtls_fprintf    fprintf
#endif /* MBEDTLS_PLATFORM_FPRINTF_MACRO */
#endif /* MBEDTLS_PLATFORM_FPRINTF_ALT */

/*
 * The function pointers for printf
 */
#if defined(MBEDTLS_PLATFORM_PRINTF_ALT)
extern int (*mbedtls_printf)(const char *format, ...);

/**
 * \brief               This function dynamically configures the snprintf
 *                      function that is called when the mbedtls_snprintf()
 *                      function is invoked by the library.
 *
 * \param printf_func   The \c printf function implementation.
 *
 * \return              \c 0 on success.
 */
int mbedtls_platform_set_printf(int (*printf_func)(const char *, ...));
#else
#undef mbedtls_printf
#if defined(MBEDTLS_PLATFORM_PRINTF_MACRO)
#define mbedtls_printf     MBEDTLS_PLATFORM_PRINTF_MACRO
#else
#define mbedtls_printf     printf
#endif /* MBEDTLS_PLATFORM_PRINTF_MACRO */
#endif /* MBEDTLS_PLATFORM_PRINTF_ALT */

/*
 * The function pointers for snprintf
 *
 * The snprintf implementation should conform to C99:
 * - it *must* always correctly zero-terminate the buffer
 *   (except when n == 0, then it must leave the buffer untouched)
 * - however it is acceptable to return -1 instead of the required length when
 *   the destination buffer is too short.
 */
#if defined(MBEDTLS_PLATFORM_HAS_NON_CONFORMING_SNPRINTF)
/* For Windows (inc. MSYS2), we provide our own fixed implementation */
int mbedtls_platform_win32_snprintf(char *s, size_t n, const char *fmt, ...);
#endif

#if defined(MBEDTLS_PLATFORM_SNPRINTF_ALT)
extern int (*mbedtls_snprintf)(char *s, size_t n, const char *format, ...);

/**
 * \brief                 This function allows configuring a custom
 *                        \c snprintf function pointer.
 *
 * \param snprintf_func   The \c snprintf function implementation.
 *
 * \return                \c 0 on success.
 */
int mbedtls_platform_set_snprintf(int (*snprintf_func)(char *s, size_t n,
                                                       const char *format, ...));
#else
#undef mbedtls_snprintf
#if defined(MBEDTLS_PLATFORM_SNPRINTF_MACRO)
#define mbedtls_snprintf   MBEDTLS_PLATFORM_SNPRINTF_MACRO
#else
#define mbedtls_snprintf   MBEDTLS_PLATFORM_STD_SNPRINTF
#endif /* MBEDTLS_PLATFORM_SNPRINTF_MACRO */
#endif /* MBEDTLS_PLATFORM_SNPRINTF_ALT */

/*
 * The function pointers for vsnprintf
 *
 * The vsnprintf implementation should conform to C99:
 * - it *must* always correctly zero-terminate the buffer
 *   (except when n == 0, then it must leave the buffer untouched)
 * - however it is acceptable to return -1 instead of the required length when
 *   the destination buffer is too short.
 */
#if defined(MBEDTLS_PLATFORM_HAS_NON_CONFORMING_VSNPRINTF)
#include <stdarg.h>
/* For Older Windows (inc. MSYS2), we provide our own fixed implementation */
int mbedtls_platform_win32_vsnprintf(char *s, size_t n, const char *fmt, va_list arg);
#endif

#if defined(MBEDTLS_PLATFORM_VSNPRINTF_ALT)
#include <stdarg.h>
extern int (*mbedtls_vsnprintf)(char *s, size_t n, const char *format, va_list arg);

/**
 * \brief   Set your own snprintf function pointer
 *
 * \param   vsnprintf_func   The \c vsnprintf function implementation
 *
 * \return  \c 0
 */
int mbedtls_platform_set_vsnprintf(int (*vsnprintf_func)(char *s, size_t n,
                                                         const char *format, va_list arg));
#else
#undef mbedtls_vsnprintf
#if defined(MBEDTLS_PLATFORM_VSNPRINTF_MACRO)
#define mbedtls_vsnprintf   MBEDTLS_PLATFORM_VSNPRINTF_MACRO
#else
#define mbedtls_vsnprintf   vsnprintf
#endif /* MBEDTLS_PLATFORM_VSNPRINTF_MACRO */
#endif /* MBEDTLS_PLATFORM_VSNPRINTF_ALT */

/*
 * The function pointers for setbuf
 */
#if defined(MBEDTLS_PLATFORM_SETBUF_ALT)
#include <stdio.h>
/**
 * \brief                  Function pointer to call for `setbuf()` functionality
 *                         (changing the internal buffering on stdio calls).
 *
 * \note                   The library calls this function to disable
 *                         buffering when reading or writing sensitive data,
 *                         to avoid having extra copies of sensitive data
 *                         remaining in stdio buffers after the file is
 *                         closed. If this is not a concern, for example if
 *                         your platform's stdio doesn't have any buffering,
 *                         you can set mbedtls_setbuf to a function that
 *                         does nothing.
 *
 *                         The library always calls this function with
 *                         `buf` equal to `NULL`.
 */
extern void (*mbedtls_setbuf)(FILE *stream, char *buf);

/**
 * \brief                  Dynamically configure the function that is called
 *                         when the mbedtls_setbuf() function is called by the
 *                         library.
 *
 * \param   setbuf_func   The \c setbuf function implementation
 *
 * \return                 \c 0
 */
int mbedtls_platform_set_setbuf(void (*setbuf_func)(
                                    FILE *stream, char *buf));
#else
#undef mbedtls_setbuf
#if defined(MBEDTLS_PLATFORM_SETBUF_MACRO)
/**
 * \brief                  Macro defining the function for the library to
 *                         call for `setbuf` functionality (changing the
 *                         internal buffering on stdio calls).
 *
 * \note                   See extra comments on the mbedtls_setbuf() function
 *                         pointer above.
 *
 * \return                 \c 0 on success, negative on error.
 */
#define mbedtls_setbuf    MBEDTLS_PLATFORM_SETBUF_MACRO
#else
#define mbedtls_setbuf    setbuf
#endif /* MBEDTLS_PLATFORM_SETBUF_MACRO */
#endif /* MBEDTLS_PLATFORM_SETBUF_ALT */

/*
 * The function pointers for exit
 */
#if defined(MBEDTLS_PLATFORM_EXIT_ALT)
extern void (*mbedtls_exit)(int status);

/**
 * \brief             This function dynamically configures the exit
 *                    function that is called when the mbedtls_exit()
 *                    function is invoked by the library.
 *
 * \param exit_func   The \c exit function implementation.
 *
 * \return            \c 0 on success.
 */
int mbedtls_platform_set_exit(void (*exit_func)(int status));
#else
#undef mbedtls_exit
#if defined(MBEDTLS_PLATFORM_EXIT_MACRO)
#define mbedtls_exit   MBEDTLS_PLATFORM_EXIT_MACRO
#else
#define mbedtls_exit   exit
#endif /* MBEDTLS_PLATFORM_EXIT_MACRO */
#endif /* MBEDTLS_PLATFORM_EXIT_ALT */

/*
 * The default exit values
 */
#if defined(MBEDTLS_PLATFORM_STD_EXIT_SUCCESS)
#define MBEDTLS_EXIT_SUCCESS MBEDTLS_PLATFORM_STD_EXIT_SUCCESS
#else
#define MBEDTLS_EXIT_SUCCESS 0
#endif
#if defined(MBEDTLS_PLATFORM_STD_EXIT_FAILURE)
#define MBEDTLS_EXIT_FAILURE MBEDTLS_PLATFORM_STD_EXIT_FAILURE
#else
#define MBEDTLS_EXIT_FAILURE 1
#endif

#if defined(MBEDTLS_ENTROPY_C) && \
    !defined(MBEDTLS_NO_PLATFORM_ENTROPY) && \
    !(defined(_WIN32) && !defined(EFIX64) && !defined(EFI32))
/* Platforms where MBEDTLS_PLATFORM_DEV_RANDOM is used
 * unless a dedicated system call is available both at
 * compile time and at run time. */
#define MBEDTLS_PLATFORM_HAVE_DEV_RANDOM
#endif

#if !defined(MBEDTLS_PLATFORM_DEV_RANDOM)
#define MBEDTLS_PLATFORM_DEV_RANDOM "/dev/random"
#endif

/* Arrange for mbedtls_platform_dev_random to always be visible to
 * Doxygen, because it's linked from the documentation of
 * MBEDTLS_PLATFORM_DEV_RANDOM and that documentation can be visible
 * even in configurations where it isn't used. */
#if defined(MBEDTLS_PLATFORM_HAVE_DEV_RANDOM) || defined(__DOXYGEN__)
/**
 * Path to a special file that returns cryptographic-quality random bytes
 * when read.
 *
 * This variable is only declared on platforms where it is used.
 * It is available when the macro `MBEDTLS_PLATFORM_HAVE_DEV_RANDOM` is defined.
 *
 * The default value is #MBEDTLS_PLATFORM_DEV_RANDOM.
 * See the documentation of this option for guidance.
 */
extern const char *mbedtls_platform_dev_random;
#endif

/*
 * The function pointers for reading from and writing a seed file to
 * Non-Volatile storage (NV) in a platform-independent way
 *
 * Only enabled when the NV seed entropy source is enabled
 */

#if !defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)

/**
 * \brief   The platform context structure.
 *
 * \note    This structure may be used to assist platform-specific
 *          setup or teardown operations.
 */
typedef struct mbedtls_platform_context {
    char MBEDTLS_PRIVATE(dummy); /**< A placeholder member, as empty structs are not portable. */
}
mbedtls_platform_context;

#else
#include "platform_alt.h"
#endif /* !MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT */

/**
 * \brief   This function performs any platform-specific initialization
 *          operations.
 *
 * \note    This function should be called before any other library functions.
 *
 *          Its implementation is platform-specific, and unless
 *          platform-specific code is provided, it does nothing.
 *
 * \note    The usage and necessity of this function is dependent on the platform.
 *
 * \param   ctx     The platform context.
 *
 * \return  \c 0 on success.
 */
int mbedtls_platform_setup(mbedtls_platform_context *ctx);
/**
 * \brief   This function performs any platform teardown operations.
 *
 * \note    This function should be called after every other Mbed TLS module
 *          has been correctly freed using the appropriate free function.
 *
 *          Its implementation is platform-specific, and unless
 *          platform-specific code is provided, it does nothing.
 *
 * \note    The usage and necessity of this function is dependent on the platform.
 *
 * \param   ctx     The platform context.
 *
 */
void mbedtls_platform_teardown(mbedtls_platform_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* platform.h */

/* ===== Retained rsa_alt_helpers.h ===== */

/**
 * \file rsa_alt_helpers.h
 *
 * \brief Context-independent RSA helper functions
 *
 *  This module declares some RSA-related helper functions useful when
 *  implementing the RSA interface. These functions are provided in a separate
 *  compilation unit in order to make it easy for designers of alternative RSA
 *  implementations to use them in their own code, as it is conceived that the
 *  functionality they provide will be necessary for most complete
 *  implementations.
 *
 *  End-users of Mbed TLS who are not providing their own alternative RSA
 *  implementations should not use these functions directly, and should instead
 *  use only the functions declared in rsa.h.
 *
 *  The interface provided by this module will be maintained through LTS (Long
 *  Term Support) branches of Mbed TLS, but may otherwise be subject to change,
 *  and must be considered an internal interface of the library.
 *
 *  There are two classes of helper functions:
 *
 *  (1) Parameter-generating helpers. These are:
 *      - mbedtls_rsa_deduce_primes
 *      - mbedtls_rsa_deduce_private_exponent
 *      - mbedtls_rsa_deduce_crt
 *       Each of these functions takes a set of core RSA parameters and
 *       generates some other, or CRT related parameters.
 *
 *  (2) Parameter-checking helpers. These are:
 *      - mbedtls_rsa_validate_params
 *      - mbedtls_rsa_validate_crt
 *      They take a set of core or CRT related RSA parameters and check their
 *      validity.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/rsa_alt_helpers.h.
 */
#ifndef MBEDTLS_RSA_ALT_HELPERS_H
#define MBEDTLS_RSA_ALT_HELPERS_H



#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Compute RSA prime moduli P, Q from public modulus N=PQ
 *                 and a pair of private and public key.
 *
 * \note           This is a 'static' helper function not operating on
 *                 an RSA context. Alternative implementations need not
 *                 overwrite it.
 *
 * \param N        RSA modulus N = PQ, with P, Q to be found
 * \param E        RSA public exponent
 * \param D        RSA private exponent
 * \param P        Pointer to MPI holding first prime factor of N on success
 * \param Q        Pointer to MPI holding second prime factor of N on success
 *
 * \return
 *                 - 0 if successful. In this case, P and Q constitute a
 *                   factorization of N.
 *                 - A non-zero error code otherwise.
 *
 * \note           It is neither checked that P, Q are prime nor that
 *                 D, E are modular inverses wrt. P-1 and Q-1. For that,
 *                 use the helper function \c mbedtls_rsa_validate_params.
 *
 */
int mbedtls_rsa_deduce_primes(mbedtls_mpi const *N, mbedtls_mpi const *E,
                              mbedtls_mpi const *D,
                              mbedtls_mpi *P, mbedtls_mpi *Q, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Compute RSA private exponent from
 *                 prime moduli and public key.
 *
 * \note           This is a 'static' helper function not operating on
 *                 an RSA context. Alternative implementations need not
 *                 overwrite it.
 *
 * \param P        First prime factor of RSA modulus
 * \param Q        Second prime factor of RSA modulus
 * \param E        RSA public exponent
 * \param D        Pointer to MPI holding the private exponent on success,
 *                 i.e. the modular inverse of E modulo LCM(P-1,Q-1).
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_MPI_ALLOC_FAILED if a memory allocation failed.
 * \return         #MBEDTLS_ERR_MPI_NOT_ACCEPTABLE if E is not coprime to P-1
 *                 and Q-1, that is, if GCD( E, (P-1)*(Q-1) ) != 1.
 * \return         #MBEDTLS_ERR_MPI_BAD_INPUT_DATA if inputs are otherwise
 *                 invalid.
 *
 * \note           This function does not check whether P and Q are primes.
 *
 */
int mbedtls_rsa_deduce_private_exponent(mbedtls_mpi const *P,
                                        mbedtls_mpi const *Q,
                                        mbedtls_mpi const *E,
                                        mbedtls_mpi *D, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Generate RSA-CRT parameters
 *
 * \note           This is a 'static' helper function not operating on
 *                 an RSA context. Alternative implementations need not
 *                 overwrite it.
 *
 * \param P        First prime factor of N
 * \param Q        Second prime factor of N
 * \param D        RSA private exponent
 * \param DP       Output variable for D modulo P-1
 * \param DQ       Output variable for D modulo Q-1
 * \param QP       Output variable for the modular inverse of Q modulo P.
 *
 * \return         0 on success, non-zero error code otherwise.
 *
 * \note           This function does not check whether P, Q are
 *                 prime and whether D is a valid private exponent.
 *
 */
int mbedtls_rsa_deduce_crt(const mbedtls_mpi *P, const mbedtls_mpi *Q,
                           const mbedtls_mpi *D, mbedtls_mpi *DP,
                           mbedtls_mpi *DQ, mbedtls_mpi *QP, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Check validity of core RSA parameters
 *
 * \note           This is a 'static' helper function not operating on
 *                 an RSA context. Alternative implementations need not
 *                 overwrite it.
 *
 * \param N        RSA modulus N = PQ
 * \param P        First prime factor of N
 * \param Q        Second prime factor of N
 * \param D        RSA private exponent
 * \param E        RSA public exponent
 * \param f_rng    PRNG to be used for primality check, or NULL
 * \param p_rng    PRNG context for f_rng, or NULL
 *
 * \return
 *                 - 0 if the following conditions are satisfied
 *                   if all relevant parameters are provided:
 *                    - P prime if f_rng != NULL (%)
 *                    - Q prime if f_rng != NULL (%)
 *                    - 1 < N = P * Q
 *                    - 1 < D, E < N
 *                    - D and E are modular inverses modulo P-1 and Q-1
 *                   (%) This is only done if MBEDTLS_GENPRIME is defined.
 *                 - A non-zero error code otherwise.
 *
 * \note           The function can be used with a restricted set of arguments
 *                 to perform specific checks only. E.g., calling it with
 *                 (-,P,-,-,-) and a PRNG amounts to a primality check for P.
 */
int mbedtls_rsa_validate_params(const mbedtls_mpi *N, const mbedtls_mpi *P,
                                const mbedtls_mpi *Q, const mbedtls_mpi *D,
                                const mbedtls_mpi *E,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Check validity of RSA CRT parameters
 *
 * \note           This is a 'static' helper function not operating on
 *                 an RSA context. Alternative implementations need not
 *                 overwrite it.
 *
 * \param P        First prime factor of RSA modulus
 * \param Q        Second prime factor of RSA modulus
 * \param D        RSA private exponent
 * \param DP       MPI to check for D modulo P-1
 * \param DQ       MPI to check for D modulo P-1
 * \param QP       MPI to check for the modular inverse of Q modulo P.
 *
 * \return
 *                 - 0 if the following conditions are satisfied:
 *                    - D = DP mod P-1 if P, D, DP != NULL
 *                    - Q = DQ mod P-1 if P, D, DQ != NULL
 *                    - QP = Q^-1 mod P if P, Q, QP != NULL
 *                 - \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if check failed,
 *                   potentially including \c MBEDTLS_ERR_MPI_XXX if some
 *                   MPI calculations failed.
 *                 - \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA if insufficient
 *                   data was provided to check DP, DQ or QP.
 *
 * \note           The function can be used with a restricted set of arguments
 *                 to perform specific checks only. E.g., calling it with the
 *                 parameters (P, -, D, DP, -, -) will check DP = D mod P-1.
 */
int mbedtls_rsa_validate_crt(const mbedtls_mpi *P,  const mbedtls_mpi *Q,
                             const mbedtls_mpi *D,  const mbedtls_mpi *DP,
                             const mbedtls_mpi *DQ, const mbedtls_mpi *QP, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* rsa_alt_helpers.h */

/* ===== Retained rsa_internal.h ===== */

/**
 * \file rsa_internal.h
 *
 * \brief Internal-only RSA public-key cryptosystem API.
 *
 * This file declares RSA-related functions that are to be used
 * only from within the Mbed TLS library itself.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/rsa_internal.h.
 */
#ifndef MBEDTLS_RSA_INTERNAL_H
#define MBEDTLS_RSA_INTERNAL_H


/**
 * \brief           Parse a PKCS#1 (ASN.1) encoded public RSA key.
 *
 * \param rsa       The RSA context where parsed data will be stored.
 * \param key       The buffer that contains the key.
 * \param keylen    The length of the key buffer in bytes.
 *
 * \return          0 on success.
 * \return          MBEDTLS_ERR_ASN1_xxx in case of ASN.1 parsing errors.
 * \return          MBEDTLS_ERR_RSA_xxx in case of RSA internal failures while
 *                  parsing data.
 * \return          MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if validity checks on the
 *                  provided key fail.
 */
int mbedtls_rsa_parse_pubkey(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics);

#endif /* rsa_internal.h */

/* ===== Retained sha1.h ===== */

/**
 * \file sha1.h
 *
 * \brief This file contains SHA-1 definitions and functions.
 *
 * The Secure Hash Algorithm 1 (SHA-1) cryptographic hash function is defined in
 * <em>FIPS 180-4: Secure Hash Standard (SHS)</em>.
 *
 * \warning   SHA-1 is considered a weak message digest and its use constitutes
 *            a security risk. We recommend considering stronger message
 *            digests instead.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/sha1.h.
 */
#ifndef MBEDTLS_SHA1_H
#define MBEDTLS_SHA1_H


#include <stddef.h>
#include <stdint.h>

/** SHA-1 input data was malformed. */
#define MBEDTLS_ERR_SHA1_BAD_INPUT_DATA                   -0x0073

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_SHA1_ALT)
// Regular implementation
//

/**
 * \brief          The SHA-1 context structure.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
typedef struct mbedtls_sha1_context {
    uint32_t MBEDTLS_PRIVATE(total)[2];          /*!< The number of Bytes processed.  */
    uint32_t MBEDTLS_PRIVATE(state)[5];          /*!< The intermediate digest state.  */
    unsigned char MBEDTLS_PRIVATE(buffer)[64];   /*!< The data block being processed. */
}
mbedtls_sha1_context;

#else
#include "sha1_alt.h"
#endif /* MBEDTLS_SHA1_ALT */

/**
 * \brief          This function initializes a SHA-1 context.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context to initialize.
 *                 This must not be \c NULL.
 *
 */
void mbedtls_sha1_init(mbedtls_sha1_context *ctx);

/**
 * \brief          This function clears a SHA-1 context.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context to clear. This may be \c NULL,
 *                 in which case this function does nothing. If it is
 *                 not \c NULL, it must point to an initialized
 *                 SHA-1 context.
 *
 */
void mbedtls_sha1_free(mbedtls_sha1_context *ctx);

/**
 * \brief          This function clones the state of a SHA-1 context.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param dst      The SHA-1 context to clone to. This must be initialized.
 * \param src      The SHA-1 context to clone from. This must be initialized.
 *
 */
void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
                        const mbedtls_sha1_context *src);

/**
 * \brief          This function starts a SHA-1 checksum calculation.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context to initialize. This must be initialized.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_sha1_starts(mbedtls_sha1_context *ctx);

/**
 * \brief          This function feeds an input buffer into an ongoing SHA-1
 *                 checksum calculation.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context. This must be initialized
 *                 and have a hash operation started.
 * \param input    The buffer holding the input data.
 *                 This must be a readable buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data \p input in Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha1_update(mbedtls_sha1_context *ctx,
                        const unsigned char *input,
                        size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function finishes the SHA-1 operation, and writes
 *                 the result to the output buffer.
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context to use. This must be initialized and
 *                 have a hash operation started.
 * \param output   The SHA-1 checksum result. This must be a writable
 *                 buffer of length \c 20 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha1_finish(mbedtls_sha1_context *ctx,
                        unsigned char output[20], MinTlsDiagnostics* diagnostics);

/**
 * \brief          SHA-1 process data block (internal use only).
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param ctx      The SHA-1 context to use. This must be initialized.
 * \param data     The data block being processed. This must be a
 *                 readable buffer of length \c 64 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_internal_sha1_process(mbedtls_sha1_context *ctx,
                                  const unsigned char data[64]);

/**
 * \brief          This function calculates the SHA-1 checksum of a buffer.
 *
 *                 The function allocates the context, performs the
 *                 calculation, and frees the context.
 *
 *                 The SHA-1 result is calculated as
 *                 output = SHA-1(input buffer).
 *
 * \warning        SHA-1 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 * \param input    The buffer holding the input data.
 *                 This must be a readable buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data \p input in Bytes.
 * \param output   The SHA-1 checksum result.
 *                 This must be a writable buffer of length \c 20 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_sha1(const unsigned char *input,
                 size_t ilen,
                 unsigned char output[20], MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_sha1.h */

/* ===== Retained sha256.h ===== */

/**
 * \file sha256.h
 *
 * \brief This file contains SHA-224 and SHA-256 definitions and functions.
 *
 * The Secure Hash Algorithms 224 and 256 (SHA-224 and SHA-256) cryptographic
 * hash functions are defined in <em>FIPS 180-4: Secure Hash Standard (SHS)</em>.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/sha256.h.
 */
#ifndef MBEDTLS_SHA256_H
#define MBEDTLS_SHA256_H


#include <stddef.h>
#include <stdint.h>

/** SHA-256 input data was malformed. */
#define MBEDTLS_ERR_SHA256_BAD_INPUT_DATA                 -0x0074

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_SHA256_ALT)
// Regular implementation
//

/**
 * \brief          The SHA-256 context structure.
 *
 *                 The structure is used both for SHA-256 and for SHA-224
 *                 checksum calculations. The choice between these two is
 *                 made in the call to mbedtls_sha256_starts().
 */
typedef struct mbedtls_sha256_context {
    unsigned char MBEDTLS_PRIVATE(buffer)[64];   /*!< The data block being processed. */
    uint32_t MBEDTLS_PRIVATE(total)[2];          /*!< The number of Bytes processed.  */
    uint32_t MBEDTLS_PRIVATE(state)[8];          /*!< The intermediate digest state.  */
}
mbedtls_sha256_context;

#else
#include "sha256_alt.h"
#endif /* MBEDTLS_SHA256_ALT */

/**
 * \brief          This function initializes a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to initialize. This must not be \c NULL.
 */
void mbedtls_sha256_init(mbedtls_sha256_context *ctx);

/**
 * \brief          This function clears a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to clear. This may be \c NULL, in which
 *                 case this function returns immediately. If it is not \c NULL,
 *                 it must point to an initialized SHA-256 context.
 */
void mbedtls_sha256_free(mbedtls_sha256_context *ctx);

/**
 * \brief          This function clones the state of a SHA-256 context.
 *
 * \param dst      The destination context. This must be initialized.
 * \param src      The context to clone. This must be initialized.
 */
void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                          const mbedtls_sha256_context *src);

/**
 * \brief          This function starts a SHA-224 or SHA-256 checksum
 *                 calculation.
 *
 * \param ctx      The context to use. This must be initialized.
 * \param is224    This determines which function to use. This must be
 *                 either \c 0 for SHA-256, or \c 1 for SHA-224.
 *
 * \note           is224 must be defined accordingly to the enabled
 *                 MBEDTLS_SHA224_C/MBEDTLS_SHA256_C symbols otherwise the
 *                 function will return #MBEDTLS_ERR_SHA512_BAD_INPUT_DATA.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-256 checksum calculation.
 *
 * \param ctx      The SHA-256 context. This must be initialized
 *                 and have a hash operation started.
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                          const unsigned char *input,
                          size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function finishes the SHA-256 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SHA-256 context. This must be initialized
 *                 and have a hash operation started.
 * \param output   The SHA-224 or SHA-256 checksum result.
 *                 This must be a writable buffer of length \c 32 bytes
 *                 for SHA-256, \c 28 bytes for SHA-224.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                          unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function processes a single data block within
 *                 the ongoing SHA-256 computation. This function is for
 *                 internal use only.
 *
 * \param ctx      The SHA-256 context. This must be initialized.
 * \param data     The buffer holding one block of data. This must
 *                 be a readable buffer of length \c 64 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx,
                                    const unsigned char data[64]);

/**
 * \brief          This function calculates the SHA-224 or SHA-256
 *                 checksum of a buffer.
 *
 *                 The function allocates the context, performs the
 *                 calculation, and frees the context.
 *
 *                 The SHA-256 result is calculated as
 *                 output = SHA-256(input buffer).
 *
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 * \param output   The SHA-224 or SHA-256 checksum result.
 *                 This must be a writable buffer of length \c 32 bytes
 *                 for SHA-256, \c 28 bytes for SHA-224.
 * \param is224    Determines which function to use. This must be
 *                 either \c 0 for SHA-256, or \c 1 for SHA-224.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha256(const unsigned char *input,
                   size_t ilen,
                   unsigned char *output,
                   int is224, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_sha256.h */

/* ===== Retained sha512.h ===== */

/**
 * \file sha512.h
 * \brief This file contains SHA-384 and SHA-512 definitions and functions.
 *
 * The Secure Hash Algorithms 384 and 512 (SHA-384 and SHA-512) cryptographic
 * hash functions are defined in <em>FIPS 180-4: Secure Hash Standard (SHS)</em>.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/sha512.h.
 */
#ifndef MBEDTLS_SHA512_H
#define MBEDTLS_SHA512_H


#include <stddef.h>
#include <stdint.h>

/** SHA-512 input data was malformed. */
#define MBEDTLS_ERR_SHA512_BAD_INPUT_DATA                 -0x0075

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_SHA512_ALT)
// Regular implementation
//

/**
 * \brief          The SHA-512 context structure.
 *
 *                 The structure is used both for SHA-384 and for SHA-512
 *                 checksum calculations. The choice between these two is
 *                 made in the call to mbedtls_sha512_starts().
 */
typedef struct mbedtls_sha512_context {
    uint64_t MBEDTLS_PRIVATE(total)[2];          /*!< The number of Bytes processed. */
    uint64_t MBEDTLS_PRIVATE(state)[8];          /*!< The intermediate digest state. */
    unsigned char MBEDTLS_PRIVATE(buffer)[128];  /*!< The data block being processed. */
    int MBEDTLS_PRIVATE(is384);                  /*!< Determines which function to use:
                                                      0: Use SHA-512, or 1: Use SHA-384. */
}
mbedtls_sha512_context;

#else
#include "sha512_alt.h"
#endif /* MBEDTLS_SHA512_ALT */

/**
 * \brief          This function initializes a SHA-512 context.
 *
 * \param ctx      The SHA-512 context to initialize. This must
 *                 not be \c NULL.
 */
void mbedtls_sha512_init(mbedtls_sha512_context *ctx);

/**
 * \brief          This function clears a SHA-512 context.
 *
 * \param ctx      The SHA-512 context to clear. This may be \c NULL,
 *                 in which case this function does nothing. If it
 *                 is not \c NULL, it must point to an initialized
 *                 SHA-512 context.
 */
void mbedtls_sha512_free(mbedtls_sha512_context *ctx);

/**
 * \brief          This function clones the state of a SHA-512 context.
 *
 * \param dst      The destination context. This must be initialized.
 * \param src      The context to clone. This must be initialized.
 */
void mbedtls_sha512_clone(mbedtls_sha512_context *dst,
                          const mbedtls_sha512_context *src);

/**
 * \brief          This function starts a SHA-384 or SHA-512 checksum
 *                 calculation.
 *
 * \param ctx      The SHA-512 context to use. This must be initialized.
 * \param is384    Determines which function to use. This must be
 *                 either \c 0 for SHA-512, or \c 1 for SHA-384.
 *
 * \note           is384 must be defined accordingly to the enabled
 *                 MBEDTLS_SHA384_C/MBEDTLS_SHA512_C symbols otherwise the
 *                 function will return #MBEDTLS_ERR_SHA512_BAD_INPUT_DATA.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha512_starts(mbedtls_sha512_context *ctx, int is384, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-512 checksum calculation.
 *
 * \param ctx      The SHA-512 context. This must be initialized
 *                 and have a hash operation started.
 * \param input    The buffer holding the input data. This must
 *                 be a readable buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha512_update(mbedtls_sha512_context *ctx,
                          const unsigned char *input,
                          size_t ilen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function finishes the SHA-512 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SHA-512 context. This must be initialized
 *                 and have a hash operation started.
 * \param output   The SHA-384 or SHA-512 checksum result.
 *                 This must be a writable buffer of length \c 64 bytes
 *                 for SHA-512, \c 48 bytes for SHA-384.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha512_finish(mbedtls_sha512_context *ctx,
                          unsigned char *output, MinTlsDiagnostics* diagnostics);

/**
 * \brief          This function processes a single data block within
 *                 the ongoing SHA-512 computation.
 *                 This function is for internal use only.
 *
 * \param ctx      The SHA-512 context. This must be initialized.
 * \param data     The buffer holding one block of data. This
 *                 must be a readable buffer of length \c 128 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_internal_sha512_process(mbedtls_sha512_context *ctx,
                                    const unsigned char data[128]);

/**
 * \brief          This function calculates the SHA-512 or SHA-384
 *                 checksum of a buffer.
 *
 *                 The function allocates the context, performs the
 *                 calculation, and frees the context.
 *
 *                 The SHA-512 result is calculated as
 *                 output = SHA-512(input buffer).
 *
 * \param input    The buffer holding the input data. This must be
 *                 a readable buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 * \param output   The SHA-384 or SHA-512 checksum result.
 *                 This must be a writable buffer of length \c 64 bytes
 *                 for SHA-512, \c 48 bytes for SHA-384.
 * \param is384    Determines which function to use. This must be either
 *                 \c 0 for SHA-512, or \c 1 for SHA-384.
 *
 * \note           is384 must be defined accordingly with the supported
 *                 symbols in the config file. If:
 *                 - is384 is 0, but \c MBEDTLS_SHA384_C is not defined, or
 *                 - is384 is 1, but \c MBEDTLS_SHA512_C is not defined
 *                 then the function will return
 *                 #MBEDTLS_ERR_SHA512_BAD_INPUT_DATA.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_sha512(const unsigned char *input,
                   size_t ilen,
                   unsigned char *output,
                   int is384, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_sha512.h */

/* ===== Retained ssl_ciphersuites_internal.h ===== */

/**
 * \file ssl_ciphersuites_internal.h
 *
 * \brief Internal part of the public "ssl_ciphersuites.h".
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_ciphersuites_internal.h.
 */
#ifndef MBEDTLS_SSL_CIPHERSUITES_INTERNAL_H
#define MBEDTLS_SSL_CIPHERSUITES_INTERNAL_H


int mbedtls_ssl_ciphersuite_uses_ec(const mbedtls_ssl_ciphersuite_t *info);
int mbedtls_ssl_ciphersuite_uses_psk(const mbedtls_ssl_ciphersuite_t *info);

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_PFS_ENABLED)

#endif /* MBEDTLS_KEY_EXCHANGE_SOME_PFS_ENABLED */

static inline int mbedtls_ssl_ciphersuite_cert_req_allowed(const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->MBEDTLS_PRIVATE(key_exchange)) {
        case MBEDTLS_KEY_EXCHANGE_RSA:
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
            return 1;

        default:
            return 0;
    }
}

static inline int mbedtls_ssl_ciphersuite_uses_srv_cert(const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->MBEDTLS_PRIVATE(key_exchange)) {
        case MBEDTLS_KEY_EXCHANGE_RSA:
        case MBEDTLS_KEY_EXCHANGE_RSA_PSK:
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
            return 1;

        default:
            return 0;
    }
}

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDHE_ENABLED)

#endif /* MBEDTLS_KEY_EXCHANGE_SOME_ECDHE_ENABLED) */

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED)
static inline int mbedtls_ssl_ciphersuite_uses_server_signature(
    const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->MBEDTLS_PRIVATE(key_exchange)) {
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
            return 1;

        default:
            return 0;
    }
}
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED */

#endif /* MBEDTLS_SSL_CIPHERSUITES_INTERNAL_H */

/* ===== Retained ssl_client.h ===== */

/**
 *  TLS 1.2 and 1.3 client-side functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_client.h.
 */

#ifndef MBEDTLS_SSL_CLIENT_H
#define MBEDTLS_SSL_CLIENT_H


/* ===== Retained ssl_misc.h ===== */

/**
 * \file ssl_misc.h
 *
 * \brief Internal functions shared by the SSL modules
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_misc.h.
 */
#ifndef MBEDTLS_SSL_MISC_H
#define MBEDTLS_SSL_MISC_H








/* ===== Retained x509_internal.h ===== */

/**
 * \file x509.h
 *
 * \brief Internal part of the public "x509.h".
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/x509_internal.h.
 */
#ifndef MBEDTLS_X509_INTERNAL_H
#define MBEDTLS_X509_INTERNAL_H




int mbedtls_x509_get_name(unsigned char **p, const unsigned char *end,
                          mbedtls_x509_name *cur, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_alg_null(unsigned char **p, const unsigned char *end,
                              mbedtls_x509_buf *alg, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_alg(unsigned char **p, const unsigned char *end,
                         mbedtls_x509_buf *alg, mbedtls_x509_buf *params, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_rsassa_pss_params(const mbedtls_x509_buf *params,
                                       mbedtls_md_type_t *md_alg, mbedtls_md_type_t *mgf_md,
                                       int *salt_len, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_sig(unsigned char **p, const unsigned char *end, mbedtls_x509_buf *sig, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_sig_alg(const mbedtls_x509_buf *sig_oid, const mbedtls_x509_buf *sig_params,
                             mbedtls_md_type_t *md_alg, mbedtls_pk_type_t *pk_alg,
                             void **sig_opts, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_time(unsigned char **p, const unsigned char *end,
                          mbedtls_x509_time *t, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_serial(unsigned char **p, const unsigned char *end,
                            mbedtls_x509_buf *serial, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_ext(unsigned char **p, const unsigned char *end,
                         mbedtls_x509_buf *ext, int tag, MinTlsDiagnostics* diagnostics);
#if !defined(MBEDTLS_X509_REMOVE_INFO)

#endif

int mbedtls_x509_set_extension(mbedtls_asn1_named_data **head, const char *oid, size_t oid_len,
                               int critical, const unsigned char *val,
                               size_t val_len);
int mbedtls_x509_write_extensions(unsigned char **p, unsigned char *start,
                                  mbedtls_asn1_named_data *first);
int mbedtls_x509_write_names(unsigned char **p, unsigned char *start,
                             mbedtls_asn1_named_data *first);
int mbedtls_x509_write_sig(unsigned char **p, unsigned char *start,
                           const char *oid, size_t oid_len,
                           unsigned char *sig, size_t size,
                           mbedtls_pk_type_t pk_alg);
int mbedtls_x509_get_ns_cert_type(unsigned char **p,
                                  const unsigned char *end,
                                  unsigned char *ns_cert_type, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_key_usage(unsigned char **p,
                               const unsigned char *end,
                               unsigned int *key_usage, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_subject_alt_name(unsigned char **p,
                                      const unsigned char *end,
                                      mbedtls_x509_sequence *subject_alt_name, MinTlsDiagnostics* diagnostics);
int mbedtls_x509_get_subject_alt_name_ext(unsigned char **p,
                                          const unsigned char *end,
                                          mbedtls_x509_sequence *subject_alt_name, MinTlsDiagnostics* diagnostics);

int mbedtls_x509_write_set_san_common(mbedtls_asn1_named_data **extensions,
                                      const mbedtls_x509_san_list *san_list);

/*
 * Check md_alg against profile
 * Return 0 if md_alg is acceptable for this profile, -1 otherwise
 */
int mbedtls_x509_profile_check_md_alg(const mbedtls_x509_crt_profile *profile,
                                      mbedtls_md_type_t md_alg);

/*
 * Check pk_alg against profile
 * Return 0 if pk_alg is acceptable for this profile, -1 otherwise
 */
int mbedtls_x509_profile_check_pk_alg(const mbedtls_x509_crt_profile *profile,
                                      mbedtls_pk_type_t pk_alg);

#endif /* MBEDTLS_X509_INTERNAL_H */


/* Shorthand for restartable ECC */

#define MBEDTLS_SSL_INITIAL_HANDSHAKE           0
#define MBEDTLS_SSL_RENEGOTIATION_IN_PROGRESS   1   /* In progress */
#define MBEDTLS_SSL_RENEGOTIATION_DONE          2   /* Done or aborted */
#define MBEDTLS_SSL_RENEGOTIATION_PENDING       3   /* Requested (server only) */

/* Faked handshake message identity for HelloRetryRequest. */
#define MBEDTLS_SSL_TLS1_3_HS_HELLO_RETRY_REQUEST (-MBEDTLS_SSL_HS_SERVER_HELLO)

/*
 * Internal identity of handshake extensions
 */
#define MBEDTLS_SSL_EXT_ID_UNRECOGNIZED                0
#define MBEDTLS_SSL_EXT_ID_SERVERNAME                  1
#define MBEDTLS_SSL_EXT_ID_SERVERNAME_HOSTNAME         1
#define MBEDTLS_SSL_EXT_ID_MAX_FRAGMENT_LENGTH         2
#define MBEDTLS_SSL_EXT_ID_STATUS_REQUEST              3
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_GROUPS            4
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_ELLIPTIC_CURVES   4
#define MBEDTLS_SSL_EXT_ID_SIG_ALG                     5
#define MBEDTLS_SSL_EXT_ID_USE_SRTP                    6
#define MBEDTLS_SSL_EXT_ID_HEARTBEAT                   7
#define MBEDTLS_SSL_EXT_ID_ALPN                        8
#define MBEDTLS_SSL_EXT_ID_SCT                         9
#define MBEDTLS_SSL_EXT_ID_CLI_CERT_TYPE              10
#define MBEDTLS_SSL_EXT_ID_SERV_CERT_TYPE             11
#define MBEDTLS_SSL_EXT_ID_PADDING                    12
#define MBEDTLS_SSL_EXT_ID_PRE_SHARED_KEY             13
#define MBEDTLS_SSL_EXT_ID_EARLY_DATA                 14
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_VERSIONS         15
#define MBEDTLS_SSL_EXT_ID_COOKIE                     16
#define MBEDTLS_SSL_EXT_ID_PSK_KEY_EXCHANGE_MODES     17
#define MBEDTLS_SSL_EXT_ID_CERT_AUTH                  18
#define MBEDTLS_SSL_EXT_ID_OID_FILTERS                19
#define MBEDTLS_SSL_EXT_ID_POST_HANDSHAKE_AUTH        20
#define MBEDTLS_SSL_EXT_ID_SIG_ALG_CERT               21
#define MBEDTLS_SSL_EXT_ID_KEY_SHARE                  22
#define MBEDTLS_SSL_EXT_ID_TRUNCATED_HMAC             23
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_POINT_FORMATS    24
#define MBEDTLS_SSL_EXT_ID_ENCRYPT_THEN_MAC           25
#define MBEDTLS_SSL_EXT_ID_EXTENDED_MASTER_SECRET     26
#define MBEDTLS_SSL_EXT_ID_SESSION_TICKET             27
#define MBEDTLS_SSL_EXT_ID_RECORD_SIZE_LIMIT          28

/* Macros used to define mask constants */
#define MBEDTLS_SSL_EXT_MASK(id)       (1ULL << (MBEDTLS_SSL_EXT_ID_##id))
/* Reset value of extension mask */
#define MBEDTLS_SSL_EXT_MASK_NONE                                              0

/* In messages containing extension requests, we should ignore unrecognized
 * extensions. In messages containing extension responses, unrecognized
 * extensions should result in handshake abortion. Messages containing
 * extension requests include ClientHello, CertificateRequest and
 * NewSessionTicket. Messages containing extension responses include
 * ServerHello, HelloRetryRequest, EncryptedExtensions and Certificate.
 *
 * RFC 8446 section 4.1.3
 *
 * The ServerHello MUST only include extensions which are required to establish
 * the cryptographic context and negotiate the protocol version.
 *
 * RFC 8446 section 4.2
 *
 * If an implementation receives an extension which it recognizes and which is
 * not specified for the message in which it appears, it MUST abort the handshake
 * with an "illegal_parameter" alert.
 */

/* Extensions that are not recognized by TLS 1.3 */
#define MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED                               \
    (MBEDTLS_SSL_EXT_MASK(SUPPORTED_POINT_FORMATS)                | \
     MBEDTLS_SSL_EXT_MASK(ENCRYPT_THEN_MAC)                       | \
     MBEDTLS_SSL_EXT_MASK(EXTENDED_MASTER_SECRET)                 | \
     MBEDTLS_SSL_EXT_MASK(SESSION_TICKET)                         | \
     MBEDTLS_SSL_EXT_MASK(TRUNCATED_HMAC)                         | \
     MBEDTLS_SSL_EXT_MASK(UNRECOGNIZED))

/* RFC 8446 section 4.2. Allowed extensions for ClientHello */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CH                                  \
    (MBEDTLS_SSL_EXT_MASK(SERVERNAME)                             | \
     MBEDTLS_SSL_EXT_MASK(MAX_FRAGMENT_LENGTH)                    | \
     MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_GROUPS)                       | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG)                                | \
     MBEDTLS_SSL_EXT_MASK(USE_SRTP)                               | \
     MBEDTLS_SSL_EXT_MASK(HEARTBEAT)                              | \
     MBEDTLS_SSL_EXT_MASK(ALPN)                                   | \
     MBEDTLS_SSL_EXT_MASK(SCT)                                    | \
     MBEDTLS_SSL_EXT_MASK(CLI_CERT_TYPE)                          | \
     MBEDTLS_SSL_EXT_MASK(SERV_CERT_TYPE)                         | \
     MBEDTLS_SSL_EXT_MASK(PADDING)                                | \
     MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY)                         | \
     MBEDTLS_SSL_EXT_MASK(PSK_KEY_EXCHANGE_MODES)                 | \
     MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_EXT_MASK(COOKIE)                                 | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS)                     | \
     MBEDTLS_SSL_EXT_MASK(CERT_AUTH)                              | \
     MBEDTLS_SSL_EXT_MASK(POST_HANDSHAKE_AUTH)                    | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG_CERT)                           | \
     MBEDTLS_SSL_EXT_MASK(RECORD_SIZE_LIMIT)                      | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/* RFC 8446 section 4.2. Allowed extensions for EncryptedExtensions */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_EE                                  \
    (MBEDTLS_SSL_EXT_MASK(SERVERNAME)                             | \
     MBEDTLS_SSL_EXT_MASK(MAX_FRAGMENT_LENGTH)                    | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_GROUPS)                       | \
     MBEDTLS_SSL_EXT_MASK(USE_SRTP)                               | \
     MBEDTLS_SSL_EXT_MASK(HEARTBEAT)                              | \
     MBEDTLS_SSL_EXT_MASK(ALPN)                                   | \
     MBEDTLS_SSL_EXT_MASK(CLI_CERT_TYPE)                          | \
     MBEDTLS_SSL_EXT_MASK(SERV_CERT_TYPE)                         | \
     MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_EXT_MASK(RECORD_SIZE_LIMIT))

/* RFC 8446 section 4.2. Allowed extensions for CertificateRequest */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CR                                  \
    (MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG)                                | \
     MBEDTLS_SSL_EXT_MASK(SCT)                                    | \
     MBEDTLS_SSL_EXT_MASK(CERT_AUTH)                              | \
     MBEDTLS_SSL_EXT_MASK(OID_FILTERS)                            | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG_CERT)                           | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/* RFC 8446 section 4.2. Allowed extensions for Certificate */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CT                                  \
    (MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SCT))

/* RFC 8446 section 4.2. Allowed extensions for ServerHello */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_SH                                  \
    (MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY)                         | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS))

/* RFC 8446 section 4.2. Allowed extensions for HelloRetryRequest */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_HRR                                 \
    (MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(COOKIE)                                 | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS))

/* RFC 8446 section 4.2. Allowed extensions for NewSessionTicket */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_NST                                 \
    (MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/*
 * Helper macros for function call with return check.
 */
/*
 * Exit when return non-zero value
 */
#define MBEDTLS_SSL_PROC_CHK(f)                               \
    do {                                                        \
        ret = (f);                                            \
        if (ret != 0)                                          \
        {                                                       \
            goto cleanup;                                       \
        }                                                       \
    } while (0)
/*
 * Exit when return negative value
 */
#define MBEDTLS_SSL_PROC_CHK_NEG(f)                           \
    do {                                                        \
        ret = (f);                                            \
        if (ret < 0)                                           \
        {                                                       \
            goto cleanup;                                       \
        }                                                       \
    } while (0)

/*
 * DTLS retransmission states, see RFC 6347 4.2.4
 *
 * The SENDING state is merged in PREPARING for initial sends,
 * but is distinct for resends.
 *
 * Note: initial state is wrong for server, but is not used anyway.
 */
#define MBEDTLS_SSL_RETRANS_PREPARING       0
#define MBEDTLS_SSL_RETRANS_SENDING         1
#define MBEDTLS_SSL_RETRANS_WAITING         2
#define MBEDTLS_SSL_RETRANS_FINISHED        3

/*
 * Allow extra bytes for record, authentication and encryption overhead:
 * counter (8) + header (5) + IV(16) + MAC (16-48) + padding (0-256).
 */

/* This macro determines whether CBC is supported. */

/* This macro determines whether a ciphersuite using a
 * stream cipher can be used. */

/* This macro determines whether the CBC construct used in TLS 1.2 is supported. */

/* This macro determines whether a ciphersuite uses Encrypt-then-MAC with CBC */

/* AEAD ciphersuites: GCM and CCM use a 128 bits tag */
#define MBEDTLS_SSL_MAC_ADD                 16

#define MBEDTLS_SSL_PADDING_ADD              0

#define MBEDTLS_SSL_MAX_CID_EXPANSION        0

#define MBEDTLS_SSL_PAYLOAD_OVERHEAD (MBEDTLS_MAX_IV_LENGTH +          \
                                      MBEDTLS_SSL_MAC_ADD +            \
                                      MBEDTLS_SSL_PADDING_ADD +        \
                                      MBEDTLS_SSL_MAX_CID_EXPANSION    \
                                      )

#define MBEDTLS_SSL_IN_PAYLOAD_LEN (MBEDTLS_SSL_PAYLOAD_OVERHEAD + \
                                    (MBEDTLS_SSL_IN_CONTENT_LEN))

#define MBEDTLS_SSL_OUT_PAYLOAD_LEN (MBEDTLS_SSL_PAYLOAD_OVERHEAD + \
                                     (MBEDTLS_SSL_OUT_CONTENT_LEN))

/* The maximum number of buffered handshake messages. */
#define MBEDTLS_SSL_MAX_BUFFERED_HS 4

/* Maximum length we can advertise as our max content length for
   RFC 6066 max_fragment_length extension negotiation purposes
   (the lesser of both sizes, if they are unequal.)
 */
#define MBEDTLS_TLS_EXT_ADV_CONTENT_LEN (                            \
        (MBEDTLS_SSL_IN_CONTENT_LEN > MBEDTLS_SSL_OUT_CONTENT_LEN)   \
        ? (MBEDTLS_SSL_OUT_CONTENT_LEN)                            \
        : (MBEDTLS_SSL_IN_CONTENT_LEN)                             \
        )

/* Maximum size in bytes of list in signature algorithms ext., RFC 5246/8446 */
#define MBEDTLS_SSL_MAX_SIG_ALG_LIST_LEN       65534

/* Minimum size in bytes of list in signature algorithms ext., RFC 5246/8446 */
#define MBEDTLS_SSL_MIN_SIG_ALG_LIST_LEN       2

/* Maximum size in bytes of list in supported elliptic curve ext., RFC 4492 */
#define MBEDTLS_SSL_MAX_CURVE_LIST_LEN         65535

#define MBEDTLS_RECEIVED_SIG_ALGS_SIZE         20

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#define MBEDTLS_TLS_SIG_NONE MBEDTLS_TLS1_3_SIG_NONE

#define MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(sig, hash) ((hash << 8) | sig)
#define MBEDTLS_SSL_TLS12_SIG_ALG_FROM_SIG_AND_HASH_ALG(alg) (alg & 0xFF)
#define MBEDTLS_SSL_TLS12_HASH_ALG_FROM_SIG_AND_HASH_ALG(alg) (alg >> 8)

#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/*
 * Check that we obey the standard's message size bounds
 */

#if MBEDTLS_SSL_IN_CONTENT_LEN > 16384
#error "Bad configuration - incoming record content too large."
#endif

#if MBEDTLS_SSL_OUT_CONTENT_LEN > 16384
#error "Bad configuration - outgoing record content too large."
#endif

#if MBEDTLS_SSL_IN_PAYLOAD_LEN > MBEDTLS_SSL_IN_CONTENT_LEN + 2048
#error "Bad configuration - incoming protected record payload too large."
#endif

#if MBEDTLS_SSL_OUT_PAYLOAD_LEN > MBEDTLS_SSL_OUT_CONTENT_LEN + 2048
#error "Bad configuration - outgoing protected record payload too large."
#endif

/* Calculate buffer sizes */

/* Note: Even though the TLS record header is only 5 bytes
   long, we're internally using 8 bytes to store the
   implicit sequence number. */
#define MBEDTLS_SSL_HEADER_LEN 13

#define MBEDTLS_SSL_IN_BUFFER_LEN  \
    ((MBEDTLS_SSL_HEADER_LEN) + (MBEDTLS_SSL_IN_PAYLOAD_LEN))

#define MBEDTLS_SSL_OUT_BUFFER_LEN  \
    ((MBEDTLS_SSL_HEADER_LEN) + (MBEDTLS_SSL_OUT_PAYLOAD_LEN))

#define MBEDTLS_CLIENT_HELLO_RANDOM_LEN 32
#define MBEDTLS_SERVER_HELLO_RANDOM_LEN 32

/** Get `ssl->in_hsfraglen`. This field is encoded as
 * mbedtls_ssl_context::badmac_seen_or_in_hsfraglen in TLS contexts,
 * and doesn't exist in DTLS contexts.
 *
 * \param[in] ssl       The SSL context to read.
 *
 * \return In TLS, the value of `in_hsfraglen`. In DTLS, 0 (handshake
 *         message defragmentation is handled different in DTLS, and
 *         does not use this field).
 */
static inline unsigned mbedtls_ssl_get_in_hsfraglen(const mbedtls_ssl_context *ssl)
{
    return ssl->badmac_seen_or_in_hsfraglen;
}

/** Set `ssl->in_hsfraglen`. This field is encoded as
 * mbedtls_ssl_context::badmac_seen_or_in_hsfraglen in TLS contexts,
 * and doesn't exist in DTLS contexts.
 *
 * \param[in,out] ssl   The SSL context to modify.
 * \param in_hsfraglen  The new value of `in_hsfraglen`.
 *
 * \return 0 in TLS, #MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED in DTLS.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
static inline int mbedtls_ssl_set_in_hsfraglen(mbedtls_ssl_context *ssl,
                                               unsigned in_hsfraglen)
{
    ssl->badmac_seen_or_in_hsfraglen = in_hsfraglen;
    return 0;
}

/*
 * TLS extension flags (for extensions with outgoing ServerHello content
 * that need it (e.g. for RENEGOTIATION_INFO the server already knows because
 * of state of the renegotiation flag, so no indicator is required)
 */
#define MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS_PRESENT (1 << 0)
#define MBEDTLS_TLS_EXT_ECJPAKE_KKPP_OK                 (1 << 1)

/**
 * \brief        This function checks if the remaining size in a buffer is
 *               greater or equal than a needed space.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed space in bytes.
 *
 * \return       Zero if the needed space is available in the buffer, non-zero
 *               otherwise.
 */
static inline int mbedtls_ssl_chk_buf_ptr(const uint8_t *cur,
                                          const uint8_t *end, size_t need)
{
    return (cur > end) || (need > (size_t) (end - cur));
}

/**
 * \brief        This macro checks if the remaining size in a buffer is
 *               greater or equal than a needed space. If it is not the case,
 *               it returns an SSL_BUFFER_TOO_SMALL error.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed space in bytes.
 *
 */
#define MBEDTLS_SSL_CHK_BUF_PTR(cur, end, need)                        \
    do {                                                                 \
        if (mbedtls_ssl_chk_buf_ptr((cur), (end), (need)) != 0) \
        {                                                                \
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL);                  \
        }                                                                \
    } while (0)

/**
 * \brief        This macro checks if the remaining length in an input buffer is
 *               greater or equal than a needed length. If it is not the case, it
 *               returns #MBEDTLS_ERR_SSL_DECODE_ERROR error and pends a
 *               #MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR alert message.
 *
 *               This is a function-like macro. It is guaranteed to evaluate each
 *               argument exactly once.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed length in bytes.
 *
 */
#define MBEDTLS_SSL_CHK_BUF_READ_PTR(cur, end, need)                          \
    do {                                                                        \
        if (mbedtls_ssl_chk_buf_ptr((cur), (end), (need)) != 0)        \
        {                                                                       \
            MBEDTLS_SSL_DEBUG_MSG(1,                                           \
                                  ("missing input data in %s", __func__));  \
            MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,   \
                                         MBEDTLS_ERR_SSL_DECODE_ERROR);       \
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);                             \
        }                                                                       \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

typedef int  mbedtls_ssl_tls_prf_cb(const unsigned char *secret, size_t slen,
                                    const char *label,
                                    const unsigned char *random, size_t rlen,
                                    unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics);

/* cipher.h exports the maximum IV, key and block length from
 * all ciphers enabled in the config, regardless of whether those
 * ciphers are actually usable in SSL/TLS. Notably, XTS is enabled
 * in the default configuration and uses 64 Byte keys, but it is
 * not used for record protection in SSL/TLS.
 *
 * In order to prevent unnecessary inflation of key structures,
 * we introduce SSL-specific variants of the max-{key,block,IV}
 * macros here which are meant to only take those ciphers into
 * account which can be negotiated in SSL/TLS.
 *
 * Since the current definitions of MBEDTLS_MAX_{KEY|BLOCK|IV}_LENGTH
 * in cipher.h are rough overapproximations of the real maxima, here
 * we content ourselves with replicating those overapproximations
 * for the maximum block and IV length, and excluding XTS from the
 * computation of the maximum key length. */
#define MBEDTLS_SSL_MAX_BLOCK_LENGTH 16
#define MBEDTLS_SSL_MAX_IV_LENGTH    16
#define MBEDTLS_SSL_MAX_KEY_LENGTH   32

/**
 * \brief   The data structure holding the cryptographic material (key and IV)
 *          used for record protection in TLS 1.3.
 */
struct mbedtls_ssl_key_set {
    /*! The key for client->server records. */
    unsigned char client_write_key[MBEDTLS_SSL_MAX_KEY_LENGTH];
    /*! The key for server->client records. */
    unsigned char server_write_key[MBEDTLS_SSL_MAX_KEY_LENGTH];
    /*! The IV  for client->server records. */
    unsigned char client_write_iv[MBEDTLS_SSL_MAX_IV_LENGTH];
    /*! The IV  for server->client records. */
    unsigned char server_write_iv[MBEDTLS_SSL_MAX_IV_LENGTH];

    size_t key_len; /*!< The length of client_write_key and
                     *   server_write_key, in Bytes. */
    size_t iv_len;  /*!< The length of client_write_iv and
                     *   server_write_iv, in Bytes. */
};
typedef struct mbedtls_ssl_key_set mbedtls_ssl_key_set;

/*
 * This structure contains the parameters only needed during handshake.
 */
struct mbedtls_ssl_handshake_params {
    /* Frequently-used boolean or byte fields (placed early to take
     * advantage of smaller code size for indirect access on Arm Thumb) */
    uint8_t resume;                     /*!<  session resume indicator*/
    uint8_t cli_exts;                   /*!< client extension presence*/

    uint8_t sni_authmode;               /*!< authmode from SNI callback     */

    /** Minimum TLS version to be negotiated.
     *
     * It is set up in the ClientHello writing preparation stage and used
     * throughout the ClientHello writing. Not relevant anymore as soon as
     * the protocol version has been negotiated thus as soon as the
     * ServerHello is received.
     * For a fresh handshake not linked to any previous handshake, it is
     * equal to the configured minimum minor version to be negotiated. When
     * renegotiating or resuming a session, it is equal to the previously
     * negotiated minor version.
     *
     * There is no maximum TLS version field in this handshake context.
     * From the start of the handshake, we need to define a current protocol
     * version for the record layer which we define as the maximum TLS
     * version to be negotiated. The `tls_version` field of the SSL context is
     * used to store this maximum value until it contains the actual
     * negotiated value.
     */
    mbedtls_ssl_protocol_version min_tls_version;

    uint8_t extended_ms;                /*!< use Extended Master Secret? */

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    unsigned char group_list_heap_allocated;
    unsigned char sig_algs_heap_allocated;
#endif

    mbedtls_ssl_ciphersuite_t const *ciphersuite_info;

    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*update_checksum)(mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*calc_verify)(const mbedtls_ssl_context *, unsigned char *, size_t *, MinTlsDiagnostics* diagnostics);
    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*calc_finished)(mbedtls_ssl_context *, unsigned char *, int, MinTlsDiagnostics* diagnostics);
    mbedtls_ssl_tls_prf_cb *tls_prf;

    /*
     * Handshake specific crypto variables
     */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
    uint16_t received_sig_algs[MBEDTLS_RECEIVED_SIG_ALGS_SIZE];
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    const uint16_t *group_list;
    const uint16_t *sig_algs;
#endif

#if !defined(MBEDTLS_USE_PSA_CRYPTO) && \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED)
    mbedtls_ecdh_context ecdh_ctx;              /*!<  ECDH key exchange       */
#endif /* !MBEDTLS_USE_PSA_CRYPTO &&
          MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    uint16_t *curves_tls_id;      /*!<  List of TLS IDs of supported elliptic curves */
#endif

    mbedtls_ssl_key_cert *key_cert;     /*!< chosen key/cert pair (server)  */
    mbedtls_ssl_key_cert *sni_key_cert; /*!< key/cert list from SNI         */
    mbedtls_x509_crt *sni_ca_chain;     /*!< trusted CAs from SNI callback  */
    mbedtls_x509_crl *sni_ca_crl;       /*!< trusted CAs CRLs from SNI      */

    struct {
        size_t total_bytes_buffered; /*!< Cumulative size of heap allocated
                                      *   buffers used for message buffering. */

        uint8_t seen_ccs;               /*!< Indicates if a CCS message has
                                         *   been seen in the current flight. */

        struct mbedtls_ssl_hs_buffer {
            unsigned is_valid      : 1;
            unsigned is_fragmented : 1;
            unsigned is_complete   : 1;
            unsigned char *data;
            size_t data_len;
        } hs[MBEDTLS_SSL_MAX_BUFFERED_HS];

        struct {
            unsigned char *data;
            size_t len;
            unsigned epoch;
        } future_record;

    } buffering;

    /*
     * Checksum contexts
     */
    mbedtls_md_context_t fin_sha256;
    mbedtls_md_context_t fin_sha384;

    uint8_t client_auth;       /*!< used to check if CertificateRequest has been
                                    received from server side. If CertificateRequest
                                    has been received, Certificate and CertificateVerify
                                    should be sent to server */
    unsigned char randbytes[MBEDTLS_CLIENT_HELLO_RANDOM_LEN +
                            MBEDTLS_SERVER_HELLO_RANDOM_LEN];
    /*!<  random bytes            */
    unsigned char premaster[MBEDTLS_PREMASTER_SIZE];
    /*!<  premaster secret        */
    size_t pmslen;                      /*!<  premaster length        */

    const unsigned char *sni_name;      /*!< raw SNI                        */
    size_t sni_name_len;                /*!< raw SNI len                    */
#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
    const mbedtls_x509_crt *dn_hints;   /*!< acceptable client cert issuers */
#endif
};

typedef struct mbedtls_ssl_hs_buffer mbedtls_ssl_hs_buffer;

/*
 * Representation of decryption/encryption transformations on records
 *
 * There are the following general types of record transformations:
 * - Stream transformations (TLS versions == 1.2 only)
 *   Transformation adding a MAC and applying a stream-cipher
 *   to the authenticated message.
 * - CBC block cipher transformations ([D]TLS versions == 1.2 only)
 *   For TLS 1.2, no IV is generated at key extraction time, but every
 *   encrypted record is explicitly prefixed by the IV with which it was
 *   encrypted.
 * - AEAD transformations ([D]TLS versions == 1.2 only)
 *   These come in two fundamentally different versions, the first one
 *   used in TLS 1.2, excluding ChaChaPoly ciphersuites, and the second
 *   one used for ChaChaPoly ciphersuites in TLS 1.2 as well as for TLS 1.3.
 *   In the first transformation, the IV to be used for a record is obtained
 *   as the concatenation of an explicit, static 4-byte IV and the 8-byte
 *   record sequence number, and explicitly prepending this sequence number
 *   to the encrypted record. In contrast, in the second transformation
 *   the IV is obtained by XOR'ing a static IV obtained at key extraction
 *   time with the 8-byte record sequence number, without prepending the
 *   latter to the encrypted record.
 *
 * Additionally, DTLS 1.2 + CID as well as TLS 1.3 use an inner plaintext
 * which allows to add flexible length padding and to hide a record's true
 * content type.
 *
 * In addition to type and version, the following parameters are relevant:
 * - The symmetric cipher algorithm to be used.
 * - The (static) encryption/decryption keys for the cipher.
 * - For stream/CBC, the type of message digest to be used.
 * - For stream/CBC, (static) encryption/decryption keys for the digest.
 * - For AEAD transformations, the size (potentially 0) of an explicit,
 *   random initialization vector placed in encrypted records.
 * - For some transformations (currently AEAD) an implicit IV. It is static
 *   and (if present) is combined with the explicit IV in a transformation-
 *   -dependent way (e.g. appending in TLS 1.2 and XOR'ing in TLS 1.3).
 * - For stream/CBC, a flag determining the order of encryption and MAC.
 * - The details of the transformation depend on the SSL/TLS version.
 * - The length of the authentication tag.
 *
 * The struct below refines this abstract view as follows:
 * - The cipher underlying the transformation is managed in
 *   cipher contexts cipher_ctx_{enc/dec}, which must have the
 *   same cipher type. The mode of these cipher contexts determines
 *   the type of the transformation in the sense above: e.g., if
 *   the type is MBEDTLS_CIPHER_AES_256_CBC resp. MBEDTLS_CIPHER_AES_192_GCM
 *   then the transformation has type CBC resp. AEAD.
 * - The cipher keys are never stored explicitly but
 *   are maintained within cipher_ctx_{enc/dec}.
 * - For stream/CBC transformations, the message digest contexts
 *   used for the MAC's are stored in md_ctx_{enc/dec}. These contexts
 *   are unused for AEAD transformations.
 * - For stream/CBC transformations, the MAC keys are not stored explicitly
 *   but maintained within md_ctx_{enc/dec}.
 * - The mac_enc and mac_dec fields are unused for EAD transformations.
 * - For transformations using an implicit IV maintained within
 *   the transformation context, its contents are stored within
 *   iv_{enc/dec}.
 * - The value of ivlen indicates the length of the IV.
 *   This is redundant in case of stream/CBC transformations
 *   which always use 0 resp. the cipher's block length as the
 *   IV length, but is needed for AEAD ciphers and may be
 *   different from the underlying cipher's block length
 *   in this case.
 * - The field fixed_ivlen is nonzero for AEAD transformations only
 *   and indicates the length of the static part of the IV which is
 *   constant throughout the communication, and which is stored in
 *   the first fixed_ivlen bytes of the iv_{enc/dec} arrays.
 * - tls_version denotes the 2-byte TLS version
 * - For stream/CBC transformations, maclen denotes the length of the
 *   authentication tag, while taglen is unused and 0.
 * - For AEAD transformations, taglen denotes the length of the
 *   authentication tag, while maclen is unused and 0.
 * - For CBC transformations, encrypt_then_mac determines the
 *   order of encryption and authentication. This field is unused
 *   in other transformations.
 *
 */
struct mbedtls_ssl_transform {
    /*
     * Session specific crypto layer
     */
    size_t minlen;                      /*!<  min. ciphertext length  */
    size_t ivlen;                       /*!<  IV length               */
    size_t fixed_ivlen;                 /*!<  Fixed part of IV (AEAD) */
    size_t maclen;                      /*!<  MAC(CBC) len            */
    size_t taglen;                      /*!<  TAG(AEAD) len           */

    unsigned char iv_enc[16];           /*!<  IV (encryption)         */
    unsigned char iv_dec[16];           /*!<  IV (decryption)         */

    mbedtls_ssl_protocol_version tls_version;

    mbedtls_cipher_context_t cipher_ctx_enc;    /*!<  encryption context      */
    mbedtls_cipher_context_t cipher_ctx_dec;    /*!<  decryption context      */

#if defined(MBEDTLS_SSL_KEEP_RANDBYTES)
    /* We need the Hello random bytes in order to re-derive keys from the
     * Master Secret and other session info and for the keying material
     * exporter in TLS 1.2.
     * See ssl_tls12_populate_transform() */
    unsigned char randbytes[MBEDTLS_SERVER_HELLO_RANDOM_LEN +
                            MBEDTLS_CLIENT_HELLO_RANDOM_LEN];
    /*!< ServerHello.random+ClientHello.random */
#endif /* defined(MBEDTLS_SSL_KEEP_RANDBYTES) */
};

/*
 * Internal representation of record frames
 *
 * Instances come in two flavors:
 * (1) Encrypted
 *     These always have data_offset = 0
 * (2) Unencrypted
 *     These have data_offset set to the amount of
 *     pre-expansion during record protection. Concretely,
 *     this is the length of the fixed part of the explicit IV
 *     used for encryption, or 0 if no explicit IV is used
 *     (e.g. for stream ciphers).
 *
 * The reason for the data_offset in the unencrypted case
 * is to allow for in-place conversion of an unencrypted to
 * an encrypted record. If the offset wasn't included, the
 * encrypted content would need to be shifted afterwards to
 * make space for the fixed IV.
 *
 */
#if MBEDTLS_SSL_CID_OUT_LEN_MAX > MBEDTLS_SSL_CID_IN_LEN_MAX
#define MBEDTLS_SSL_CID_LEN_MAX MBEDTLS_SSL_CID_OUT_LEN_MAX
#else
#define MBEDTLS_SSL_CID_LEN_MAX MBEDTLS_SSL_CID_IN_LEN_MAX
#endif

typedef struct {
    uint8_t ctr[MBEDTLS_SSL_SEQUENCE_NUMBER_LEN];  /* In TLS:  The implicit record sequence number.
                                                    * In DTLS: The 2-byte epoch followed by
                                                    *          the 6-byte sequence number.
                                                    * This is stored as a raw big endian byte array
                                                    * as opposed to a uint64_t because we rarely
                                                    * need to perform arithmetic on this, but do
                                                    * need it as a Byte array for the purpose of
                                                    * MAC computations.                             */
    uint8_t type;           /* The record content type.                      */
    uint8_t ver[2];         /* SSL/TLS version as present on the wire.
                             * Convert to internal presentation of versions
                             * using mbedtls_ssl_read_version() and
                             * mbedtls_ssl_write_version().
                             * Keep wire-format for MAC computations.        */

    unsigned char *buf;     /* Memory buffer enclosing the record content    */
    size_t buf_len;         /* Buffer length                                 */
    size_t data_offset;     /* Offset of record content                      */
    size_t data_len;        /* Length of record content                      */

} mbedtls_record;

/*
 * List of certificate + private key pairs
 */
struct mbedtls_ssl_key_cert {
    mbedtls_x509_crt *cert;                 /*!< cert                       */
    mbedtls_pk_context *key;                /*!< private key                */
    mbedtls_ssl_key_cert *next;             /*!< next key/cert pair         */
};

/**
 * \brief Given an SSL context and its associated configuration, write the TLS
 *        1.2 specific extensions of the ClientHello message.
 *
 * \param[in]   ssl     SSL context
 * \param[in]   buf     Base address of the buffer where to write the extensions
 * \param[in]   end     End address of the buffer where to write the extensions
 * \param       uses_ec Whether one proposed ciphersuite uses an elliptic curve
 *                      (<> 0) or not ( 0 ).
 * \param[out]  out_len Length of the data written into the buffer \p buf
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_tls12_write_client_hello_exts(mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              const unsigned char *end,
                                              int uses_ec,
                                              size_t *out_len, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_SSL_PROTO_TLS1_2) && \
    defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)

#endif /* MBEDTLS_SSL_PROTO_TLS1_2 &&
          MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

/**
 * \brief           Free referenced items in an SSL transform context and clear
 *                  memory
 *
 * \param transform SSL transform context
 */
void mbedtls_ssl_transform_free(mbedtls_ssl_transform *transform);

/**
 * \brief           Free referenced items in an SSL handshake context and clear
 *                  memory
 *
 * \param ssl       SSL context
 */
void mbedtls_ssl_handshake_free(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handshake_client_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handshake_server_step(mbedtls_ssl_context *ssl);
void mbedtls_ssl_handshake_wrapup(mbedtls_ssl_context *ssl);

static inline void mbedtls_ssl_handshake_set_state(mbedtls_ssl_context *ssl,
                                                   mbedtls_ssl_states state)
{
    MBEDTLS_SSL_DEBUG_MSG(3, ("handshake state: %d (%s) -> %d (%s)",
                              ssl->state, mbedtls_ssl_states_str((mbedtls_ssl_states) ssl->state),
                              (int) state, mbedtls_ssl_states_str(state)));
    ssl->state = (int) state;
}

static inline void mbedtls_ssl_handshake_increment_state(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_handshake_set_state(ssl, (mbedtls_ssl_states) (ssl->state + 1));
}

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_reset_checksum(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_derive_keys(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handle_message_type(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_prepare_handshake_record(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_update_handshake_status(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief       Update record layer
 *
 *              This function roughly separates the implementation
 *              of the logic of (D)TLS from the implementation
 *              of the secure transport.
 *
 * \param  ssl              The SSL context to use.
 * \param  update_hs_digest This indicates if the handshake digest
 *                          should be automatically updated in case
 *                          a handshake message is found.
 *
 * \return      0 or non-zero error code.
 *
 * \note        A clarification on what is called 'record layer' here
 *              is in order, as many sensible definitions are possible:
 *
 *              The record layer takes as input an untrusted underlying
 *              transport (stream or datagram) and transforms it into
 *              a serially multiplexed, secure transport, which
 *              conceptually provides the following:
 *
 *              (1) Three datagram based, content-agnostic transports
 *                  for handshake, alert and CCS messages.
 *              (2) One stream- or datagram-based transport
 *                  for application data.
 *              (3) Functionality for changing the underlying transform
 *                  securing the contents.
 *
 *              The interface to this functionality is given as follows:
 *
 *              a Updating
 *                [Currently implemented by mbedtls_ssl_read_record]
 *
 *                Check if and on which of the four 'ports' data is pending:
 *                Nothing, a controlling datagram of type (1), or application
 *                data (2). In any case data is present, internal buffers
 *                provide access to the data for the user to process it.
 *                Consumption of type (1) datagrams is done automatically
 *                on the next update, invalidating that the internal buffers
 *                for previous datagrams, while consumption of application
 *                data (2) is user-controlled.
 *
 *              b Reading of application data
 *                [Currently manual adaption of ssl->in_offt pointer]
 *
 *                As mentioned in the last paragraph, consumption of data
 *                is different from the automatic consumption of control
 *                datagrams (1) because application data is treated as a stream.
 *
 *              c Tracking availability of application data
 *                [Currently manually through decreasing ssl->in_msglen]
 *
 *                For efficiency and to retain datagram semantics for
 *                application data in case of DTLS, the record layer
 *                provides functionality for checking how much application
 *                data is still available in the internal buffer.
 *
 *              d Changing the transformation securing the communication.
 *
 *              Given an opaque implementation of the record layer in the
 *              above sense, it should be possible to implement the logic
 *              of (D)TLS on top of it without the need to know anything
 *              about the record layer's internals. This is done e.g.
 *              in all the handshake handling functions, and in the
 *              application data reading function mbedtls_ssl_read.
 *
 * \note        The above tries to give a conceptual picture of the
 *              record layer, but the current implementation deviates
 *              from it in some places. For example, our implementation of
 *              the update functionality through mbedtls_ssl_read_record
 *              discards datagrams depending on the current state, which
 *              wouldn't fall under the record layer's responsibility
 *              following the above definition.
 *
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_read_record(mbedtls_ssl_context *ssl,
                            unsigned update_hs_digest, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_fetch_input(mbedtls_ssl_context *ssl, size_t nb_want, MinTlsDiagnostics* diagnostics);

/*
 * Write handshake message header
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_start_handshake_msg(mbedtls_ssl_context *ssl, unsigned char hs_type,
                                    unsigned char **buf, size_t *buf_len);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_handshake_msg_ext(mbedtls_ssl_context *ssl,
                                        int update_checksum,
                                        int force_flush, MinTlsDiagnostics* diagnostics);
static inline int mbedtls_ssl_write_handshake_msg(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ssl_write_handshake_msg_ext(ssl, 1 /* update checksum */, 1 /* force flush */, diagnostics));
}

/*
 * Write handshake message tail
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_finish_handshake_msg(mbedtls_ssl_context *ssl,
                                     size_t buf_len, size_t msg_len, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_record(mbedtls_ssl_context *ssl, int force_flush, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_flush_output(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_change_cipher_spec(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_change_cipher_spec(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_finished(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_finished(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

void mbedtls_ssl_optimize_checksum(mbedtls_ssl_context *ssl,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_add_hs_hdr_to_checksum(mbedtls_ssl_context *ssl,
                                       unsigned hs_type,
                                       size_t total_hs_len, MinTlsDiagnostics* diagnostics);

unsigned char mbedtls_ssl_sig_from_pk(mbedtls_pk_context *pk);

mbedtls_pk_type_t mbedtls_ssl_pk_alg_from_sig(unsigned char sig);

mbedtls_md_type_t mbedtls_ssl_md_alg_from_hash(unsigned char hash);
unsigned char mbedtls_ssl_hash_from_md_alg(int md);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_curve_tls_id(const mbedtls_ssl_context *ssl, uint16_t tls_id, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_curve(const mbedtls_ssl_context *ssl, mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics);

/**
 * \brief Return \c mbedtls_ecp_group_id for the specified TLS ID.
 *
 * \param tls_id    The TLS ID to look for
 * \return          Proper \c mbedtls_ecp_group_id if the TLS ID is supported,
 *                  or MBEDTLS_ECP_DP_NONE otherwise
 */
mbedtls_ecp_group_id mbedtls_ssl_get_ecp_group_id_from_tls_id(uint16_t tls_id);

/**
 * \brief Return TLS ID for the specified \c mbedtls_ecp_group_id.
 *
 * \param grp_id    The \c mbedtls_ecp_group_id ID to look for
 * \return          Proper TLS ID if the \c mbedtls_ecp_group_id is supported,
 *                  or 0 otherwise
 */
uint16_t mbedtls_ssl_get_tls_id_from_ecp_group_id(mbedtls_ecp_group_id grp_id);

static inline mbedtls_pk_context *mbedtls_ssl_own_key(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_key_cert *key_cert;

    if (ssl->handshake != NULL && ssl->handshake->key_cert != NULL) {
        key_cert = ssl->handshake->key_cert;
    } else {
        key_cert = ssl->conf->key_cert;
    }

    return key_cert == NULL ? NULL : key_cert->key;
}

static inline mbedtls_x509_crt *mbedtls_ssl_own_cert(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_key_cert *key_cert;

    if (ssl->handshake != NULL && ssl->handshake->key_cert != NULL) {
        key_cert = ssl->handshake->key_cert;
    } else {
        key_cert = ssl->conf->key_cert;
    }

    return key_cert == NULL ? NULL : key_cert->cert;
}

/*
 * Verify a certificate.
 *
 * [in/out] ssl: misc. things read
 *               ssl->session_negotiate->verify_result updated
 * [in] authmode: one of MBEDTLS_SSL_VERIFY_{NONE,OPTIONAL,REQUIRED}
 * [in] chain: the certificate chain to verify (ie the peer's chain)
 * [in] ciphersuite_info: For TLS 1.2, this session's ciphersuite;
 *                        for TLS 1.3, may be left NULL.
 * [in] rs_ctx: restart context if restartable ECC is in use;
 *              leave NULL for no restartable behaviour.
 *
 * Return:
 * - 0 if the handshake should continue. Depending on the
 *   authmode it means:
 *   - REQUIRED: the certificate was found to be valid, trusted & acceptable.
 *     ssl->session_negotiate->verify_result is 0.
 *   - OPTIONAL: the certificate may or may not be acceptable, but
 *     ssl->session_negotiate->verify_result was updated with the result.
 *   - NONE: the certificate wasn't even checked.
 * - MBEDTLS_ERR_X509_CERT_VERIFY_FAILED or MBEDTLS_ERR_SSL_BAD_CERTIFICATE if
 *   the certificate was found to be invalid/untrusted/unacceptable and the
 *   handshake should be aborted (can only happen with REQUIRED).
 * - another error code if another error happened (out-of-memory, etc.)
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_verify_certificate(mbedtls_ssl_context *ssl,
                                   int authmode,
                                   mbedtls_x509_crt *chain,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info,
                                   void *rs_ctx, MinTlsDiagnostics* diagnostics);

/*
 * Check usage of a certificate wrt usage extensions:
 * keyUsage and extendedKeyUsage.
 * (Note: nSCertType is deprecated and not standard, we don't check it.)
 *
 * Note: if tls_version is 1.3, ciphersuite is ignored and can be NULL.
 *
 * Note: recv_endpoint is the receiver's endpoint.
 *
 * Return 0 if everything is OK, -1 if not.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_cert_usage(const mbedtls_x509_crt *cert,
                                 const mbedtls_ssl_ciphersuite_t *ciphersuite,
                                 int recv_endpoint,
                                 mbedtls_ssl_protocol_version tls_version,
                                 uint32_t *flags, MinTlsDiagnostics* diagnostics);

void mbedtls_ssl_write_version(unsigned char version[2], int transport,
                               mbedtls_ssl_protocol_version tls_version);
uint16_t mbedtls_ssl_read_version(const unsigned char version[2],
                                  int transport);

static inline size_t mbedtls_ssl_in_hdr_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);

    {
        return 5;
    }
}

static inline size_t mbedtls_ssl_out_hdr_len(const mbedtls_ssl_context *ssl)
{
    return (size_t) (ssl->out_iv - ssl->out_hdr);
}

static inline size_t mbedtls_ssl_hs_hdr_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);
    return 4;
}

/* The hash buffer must have at least MBEDTLS_MD_MAX_SIZE bytes of length. */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_get_key_exchange_md_tls1_2(mbedtls_ssl_context *ssl,
                                           unsigned char *hash, size_t *hashlen,
                                           unsigned char *data, size_t data_len,
                                           mbedtls_md_type_t md_alg, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

void mbedtls_ssl_transform_init(mbedtls_ssl_transform *transform);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_encrypt_buf(mbedtls_ssl_context *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_decrypt_buf(mbedtls_ssl_context const *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec, MinTlsDiagnostics* diagnostics);

/* Length of the "epoch" field in the record header */
static inline size_t mbedtls_ssl_ep_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);
    return 0;
}

void mbedtls_ssl_set_timer(mbedtls_ssl_context *ssl, uint32_t millisecs);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_timer(mbedtls_ssl_context *ssl);

void mbedtls_ssl_reset_in_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_update_in_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_reset_out_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_update_out_pointers(mbedtls_ssl_context *ssl,
                                     mbedtls_ssl_transform *transform);

/*
 * Send pending alert
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handle_pending_alert(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/*
 * Set pending fatal alert flag.
 */
void mbedtls_ssl_pend_fatal_alert(mbedtls_ssl_context *ssl,
                                  unsigned char alert_type,
                                  int alert_reason);

/* Alias of mbedtls_ssl_pend_fatal_alert */
#define MBEDTLS_SSL_PEND_FATAL_ALERT(type, user_return_value)         \
    mbedtls_ssl_pend_fatal_alert(ssl, type, user_return_value)

void mbedtls_ssl_handshake_wrapup_free_hs_transform(mbedtls_ssl_context *ssl);

/**
 * ssl utils functions for checking configuration.
 */

static inline int mbedtls_ssl_conf_is_tls12_only(const mbedtls_ssl_config *conf)
{
    return conf->min_tls_version == MBEDTLS_SSL_VERSION_TLS1_2 &&
           conf->max_tls_version == MBEDTLS_SSL_VERSION_TLS1_2;
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
/*
 * Write Signature Algorithm extension
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_sig_alg_ext(mbedtls_ssl_context *ssl, unsigned char *buf,
                                  const unsigned char *end, size_t *out_len, MinTlsDiagnostics* diagnostics);

#endif

/*
 * Return supported groups.
 *
 * In future, invocations can be changed to ssl->conf->group_list
 * when mbedtls_ssl_conf_curves() is deleted.
 *
 * ssl->handshake->group_list is either a translation of curve_list to IANA TLS group
 * identifiers when mbedtls_ssl_conf_curves() has been used, or a pointer to
 * ssl->conf->group_list when mbedtls_ssl_conf_groups() has been more recently invoked.
 *
 */
static inline const uint16_t *mbedtls_ssl_get_groups(const mbedtls_ssl_context *ssl)
{
    #if defined(MBEDTLS_DEPRECATED_REMOVED) || !defined(MBEDTLS_ECP_C)
    return ssl->conf->group_list;
#else
    if ((ssl->handshake != NULL) && (ssl->handshake->group_list != NULL)) {
        return ssl->handshake->group_list;
    } else {
        return ssl->conf->group_list;
    }
    #endif
}

/*
 * Helper functions for NamedGroup.
 */
static inline int mbedtls_ssl_tls12_named_group_is_ecdhe(uint16_t named_group)
{
    /*
     * RFC 8422 section 5.1.1
     */
    return named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519    ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP256R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP384R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP512R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X448      ||
           /* Below deprecated curves should be removed with notice to users */
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP192K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP192R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP224K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP224R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1;
}

/*
 * Return supported signature algorithms.
 *
 * In future, invocations can be changed to ssl->conf->sig_algs when
 * mbedtls_ssl_conf_sig_hashes() is deleted.
 *
 * ssl->handshake->sig_algs is either a translation of sig_hashes to IANA TLS
 * signature algorithm identifiers when mbedtls_ssl_conf_sig_hashes() has been
 * used, or a pointer to ssl->conf->sig_algs when mbedtls_ssl_conf_sig_algs() has
 * been more recently invoked.
 *
 */
static inline const uint16_t *mbedtls_ssl_get_sig_algs(
    const mbedtls_ssl_context *ssl)
{
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    if (ssl->handshake != NULL &&
        ssl->handshake->sig_algs_heap_allocated == 1 &&
        ssl->handshake->sig_algs != NULL) {
        return ssl->handshake->sig_algs;
    }
#endif
    return ssl->conf->sig_algs;

#else

    ((void) ssl);
    return NULL;
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
static inline int mbedtls_ssl_sig_alg_is_offered(const mbedtls_ssl_context *ssl,
                                                 uint16_t proposed_sig_alg)
{
    const uint16_t *sig_alg = mbedtls_ssl_get_sig_algs(ssl);
    if (sig_alg == NULL) {
        return 0;
    }

    for (; *sig_alg != MBEDTLS_TLS_SIG_NONE; sig_alg++) {
        if (*sig_alg == proposed_sig_alg) {
            return 1;
        }
    }
    return 0;
}

static inline int mbedtls_ssl_get_pk_type_and_md_alg_from_sig_alg(
    uint16_t sig_alg, mbedtls_pk_type_t *pk_type, mbedtls_md_type_t *md_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    *pk_type = mbedtls_ssl_pk_alg_from_sig(sig_alg & 0xff);
    *md_alg = mbedtls_ssl_md_alg_from_hash((sig_alg >> 8) & 0xff);

    if (*pk_type != MBEDTLS_PK_NONE && *md_alg != MBEDTLS_MD_NONE) {
        MINTLS_RETURN(0);
    }

    switch (sig_alg) {
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256:
            *md_alg = MBEDTLS_MD_SHA256;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384:
            *md_alg = MBEDTLS_MD_SHA384;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512:
            *md_alg = MBEDTLS_MD_SHA512;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }
    MINTLS_RETURN(0);
}

static inline int mbedtls_ssl_tls12_sig_alg_is_supported(
    const uint16_t sig_alg)
{
    /* High byte is hash */
    unsigned char hash = MBEDTLS_BYTE_1(sig_alg);
    unsigned char sig = MBEDTLS_BYTE_0(sig_alg);

    switch (hash) {

        case MBEDTLS_SSL_HASH_SHA1:
            break;

        case MBEDTLS_SSL_HASH_SHA256:
            break;

        case MBEDTLS_SSL_HASH_SHA384:
            break;

        case MBEDTLS_SSL_HASH_SHA512:
            break;

        default:
            return 0;
    }

    switch (sig) {
        case MBEDTLS_SSL_SIG_RSA:
            break;

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
        case MBEDTLS_SSL_SIG_ECDSA:
            break;
#endif

        default:
            return 0;
    }

    return 1;
}

static inline int mbedtls_ssl_sig_alg_is_supported(
    const mbedtls_ssl_context *ssl,
    const uint16_t sig_alg)
{

    if (ssl->tls_version == MBEDTLS_SSL_VERSION_TLS1_2) {
        return mbedtls_ssl_tls12_sig_alg_is_supported(sig_alg);
    }

    ((void) ssl);
    ((void) sig_alg);
    return 0;
}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/**
 * \brief       TLS record protection modes
 */
typedef enum {
    MBEDTLS_SSL_MODE_STREAM = 0,
    MBEDTLS_SSL_MODE_CBC,
    MBEDTLS_SSL_MODE_CBC_ETM,
    MBEDTLS_SSL_MODE_AEAD
} mbedtls_ssl_mode_t;

mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_transform(
    const mbedtls_ssl_transform *transform);

#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_ciphersuite(
    int encrypt_then_mac,
    const mbedtls_ssl_ciphersuite_t *suite);
#else
mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_ciphersuite(
    const mbedtls_ssl_ciphersuite_t *suite);
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */

#if defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_tls13_read_public_xxdhe_share(mbedtls_ssl_context *ssl,
                                              const unsigned char *buf,
                                              size_t buf_len);

#endif

/**
 * \brief Validate cipher suite against config in SSL context.
 *
 * \param ssl              SSL context
 * \param suite_info       Cipher suite to validate
 * \param min_tls_version  Minimal TLS version to accept a cipher suite
 * \param max_tls_version  Maximal TLS version to accept a cipher suite
 *
 * \return 0 if valid, negative value otherwise.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_validate_ciphersuite(
    const mbedtls_ssl_context *ssl,
    const mbedtls_ssl_ciphersuite_t *suite_info,
    mbedtls_ssl_protocol_version min_tls_version,
    mbedtls_ssl_protocol_version max_tls_version);

/** Get the host name from the SSL context.
 *
 * \param[in]   ssl     SSL context
 *
 * \return The \p hostname pointer from the SSL context.
 *         \c NULL if mbedtls_ssl_set_hostname() has never been called on
 *         \p ssl or if it was last called with \p NULL.
 */
const char *mbedtls_ssl_get_hostname_pointer(const mbedtls_ssl_context *ssl);

#endif /* ssl_misc.h */


#include <stddef.h>

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_client_hello(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

#endif /* MBEDTLS_SSL_CLIENT_H */

/* ===== Retained ssl_debug_helpers.h ===== */

/**
 * \file ssl_debug_helpers.h
 *
 * \brief Automatically generated helper functions for debugging
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_debug_helpers.h.
 */

#ifndef MBEDTLS_SSL_DEBUG_HELPERS_H
#define MBEDTLS_SSL_DEBUG_HELPERS_H


#define MBEDTLS_SSL_PRINT_EXTS(level, hs_msg_type, extension_mask)

#define MBEDTLS_SSL_PRINT_EXT(level, hs_msg_type, extension_type, extra)

#endif /* MBEDTLS_SSL_DEBUG_HELPERS_H */

/* ===== Retained version.h ===== */

/**
 * \file version.h
 *
 * \brief Run-time version information
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/version.h.
 */
/*
 * This set of run-time variables can be used to determine the version number of
 * the Mbed TLS library used. Compile-time version defines for the same can be
 * found in build_info.h
 */
#ifndef MBEDTLS_VERSION_H
#define MBEDTLS_VERSION_H


#if defined(MBEDTLS_VERSION_C)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the version number.
 *
 * \return          The constructed version number in the format
 *                  MMNNPP00 (Major, Minor, Patch).
 */
unsigned int mbedtls_version_get_number(void);

/**
 * Get the version string ("x.y.z").
 *
 * \param string    The string that will receive the value.
 *                  (Should be at least 9 bytes in size)
 */
void mbedtls_version_get_string(char *string);

/**
 * Get the full version string ("Mbed TLS x.y.z").
 *
 * \param string    The string that will receive the value. The Mbed TLS version
 *                  string will use 18 bytes AT MOST including a terminating
 *                  null byte.
 *                  (So the buffer should be at least 18 bytes to receive this
 *                  version string).
 */
void mbedtls_version_get_string_full(char *string);

/**
 * \brief           Check if support for a feature was compiled into this
 *                  Mbed TLS binary. This allows you to see at runtime if the
 *                  library was for instance compiled with or without
 *                  Multi-threading support.
 *
 * \note            only checks against defines in the sections "System
 *                  support", "Mbed TLS modules" and "Mbed TLS feature
 *                  support" in mbedtls_config.h
 *
 * \param feature   The string for the define to check (e.g. "MBEDTLS_AES_C")
 *
 * \return          0 if the feature is present,
 *                  -1 if the feature is not present and
 *                  -2 if support for feature checking as a whole was not
 *                  compiled in.
 */
int mbedtls_version_check_feature(const char *feature);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_VERSION_C */

#endif /* version.h */

#endif /* OSCONFIG_MIN_TLS_CORE_H */
