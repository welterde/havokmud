#include "config.h"
#include "environment.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "protos.h"
#include "externs.h"

/*
 * external vars
 */



void PrintSkills(struct char_data *ch, int level,
                 const struct skillset *skills, int skillCount, char *buffer);
int LearnSkill(struct char_data *ch, const struct skillset *skills, 
               int skillCount, char *arg, int level, char *teacher, int charge);
int guildmaster_skeleton(struct char_data *ch, int cmd, char *arg,
                         struct char_data *mob, int type, int class_bit, 
                         int level_ind);


#define SPELL_SPECIAL_COST 100000       /* 100k to specialize per spell */

/*
 * extern procedures
 */

/** @author:      Banon (GH)
 ** @date:        June 19th, 2004
 ** @description: Lets get rid of redundant code.
 **               10 guildmasters with all the same code?? Blah!
 **/

int guildmaster_skeleton(struct char_data *ch, int cmd, char *arg,
                         struct char_data *mob, int type, int class_bit,
                         int level_ind)
{                               /* proc_type proc) */

    char            guildmastername[128];
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];
    struct char_data *guildmaster;

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }

    /*
     * 170->Practice,164->Practise, 243->gain
     */

    if (cmd == CMD_practice || cmd == CMD_study || cmd == CMD_gain) {
		/* oldSendOutput(ch,"Gain:%d  Study:%d Practice:%d",
		 *  CMD_gain, CMD_study, CMD_practice);
         */

        sprintf(guildmastername,"%s",classes[level_ind].name);

        sprintf(guildmastername, "%s", classes[level_ind].name);

        if (!mob) {
            /*
             * will there ever be a time that there is no mob? we need to
             * fix below
             */
            guildmaster = FindMobInRoomWithFunction(ch->in_room,
                                                    barbarian_guildmaster);
        } else {
            guildmaster = mob;
        }

        if (!HasClass(ch, class_bit)) {
            sprintf(buf, "You're not a %s!", guildmastername);
            do_mobTell2(ch, guildmaster, buf);
            return (TRUE);
        }

        if (GET_LEVEL(ch, level_ind) > GetMaxLevel(guildmaster)) {
            do_mobTell2(ch, guildmaster,
                        "You must learn from another, I can no longer train "
                        "you");
            return (TRUE);
        }

        /*
         * gain
         */
        if (cmd == 243) {
            if (GET_EXP(ch) <
                classes[level_ind].levels[GET_LEVEL(ch, level_ind) + 1].exp) {
                do_mobTell2(ch, guildmaster, "You're not ready to gain yet!");
                /*
                 * send_to_char("Your not ready to gain yet!", ch);
                 */
                return (FALSE);
            } 
            
            if (level_ind == MONK_LEVEL_IND) {
                /*
                 * Special case: Monks have to do a challenge to gain
                 * after level 9
                 */
                if (GET_LEVEL(ch, MONK_LEVEL_IND) > 9) {
                    send_to_char("You must fight another monk for this "
                                 "title.\n\r", ch);
                }
                return (TRUE);
            } 
            
            if (level_ind == DRUID_LEVEL_IND) {
                /*
                 * Special case: Monks have to do a challenge to gain
                 * after level 9
                 */
                if (GET_LEVEL(ch, DRUID_LEVEL_IND) > 9) {
                    send_to_char("You must fight another druid for this "
                                 "title.\n\r", ch);
                }
                return (TRUE);
            } 

            /*
             * else just gain the level
             */
            GainLevel(ch, level_ind);
            return (TRUE);
        }

        if (!arg || !*arg) {
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of these spells:\n\r\n\r");
            strcat(buffer, buf);
            PrintSkills(ch, GET_LEVEL(ch, level_ind),
                        classes[level_ind].skills,
                        classes[level_ind].skillCount, buffer);
            if (ch->specials.remortclass == level_ind + 1 &&
                classes[level_ind].mainskillCount) {
                sprintf(buf, "\n\rSince you picked %s as your main "
                        "class, you get these bonus skills:\n\r\n\r",
                        guildmastername);
                strcat(buffer, buf);
                PrintSkills(ch, GET_LEVEL(ch, level_ind),
                            classes[level_ind].mainskills, 
                            classes[level_ind].mainskillCount, buffer);
            }
            page_string(ch->desc, buffer, 1);
        } else {
            if (LearnSkill(ch, classes[level_ind].skills, 
                           classes[level_ind].skillCount, arg,
                           GET_LEVEL(ch, level_ind), guildmastername, 0)) {
                return (TRUE);
            }

            if (ch->specials.remortclass == level_ind + 1 &&
                classes[level_ind].mainskillCount &&
                LearnSkill(ch, classes[level_ind].mainskills, 
                           classes[level_ind].mainskillCount, arg,
                           GET_LEVEL(ch, level_ind), guildmastername, 0)) {
                return (TRUE);
            }

            do_mobTell2(ch, guildmaster, "I do not know of that skill!");
        }
        return (TRUE);
    }
    return (FALSE);
}


int sailor(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    char            buf[256];
    static char    *n_skills[] = {
        "swim",
        "\n",
    };
    int             percent = 0,
                    number = 0;
    int             charge,
                    sk_num;

    if (!AWAKE(ch)) {
        return (FALSE);
    }
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
        return (FALSE);
    }

    if (!ch->skills) {
        return (FALSE);
    }
    if (check_soundproof(ch)) {
        return (FALSE);
    }

    arg = skip_spaces(arg);

    if ((cmd == 164) || (cmd == 170)) {
        if (!arg) {
            sprintf(buf, " swim   :  %s\n\r",
                    how_good(ch->skills[SKILL_SWIM].learned));
            send_to_char(buf, ch);
            return (TRUE);
        } else {
            number = old_search_block(arg, 0, strlen(arg), n_skills, FALSE);
            send_to_char("The sailor says ", ch);
            if (number == -1) {
                send_to_char("'I do not know of this skill.'\n\r", ch);
                return (TRUE);
            }
            charge = GetMaxLevel(ch) * 100;
            switch (number) {
            case 0:
            case 1:
                sk_num = SKILL_SWIM;
                break;
            default:
                Log("Strangeness in sailor (%d)", number);
                send_to_char("'Ack!  I feel faint!'\n\r", ch);
                return (FALSE);
                break;
            }
        }

        if (GET_GOLD(ch) < charge) {
            send_to_char("'Ah, but you do not have enough money to pay.'\n\r",
                         ch);
            return (TRUE);
        }

        if (ch->skills[sk_num].learned > 60) {
            send_to_char("You must learn from practice and experience now.\n\r",
                         ch);
            return (TRUE);
        }

        if (ch->specials.spells_to_learn <= 0) {
            send_to_char("'You must first earn more practices you already "
                         "have.\n\r", ch);
            return (TRUE);
        }

        GET_GOLD(ch) -= charge;
        send_to_char("'We will now begin.'\n\r", ch);
        ch->specials.spells_to_learn--;

        percent = ch->skills[sk_num].learned + int_app[(int)GET_INT(ch)].learn;
        ch->skills[sk_num].learned = MIN(95, percent);

        if (ch->skills[sk_num].learned >= 95) {
            send_to_char("'You are now a master of this art.'\n\r", ch);
            return (TRUE);
        }
    }
    return (FALSE);
}

int loremaster(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    /*
     * 170->Practice,164->Practise,
     */
    if (cmd == 164 || cmd == 170 || cmd == 582) {
        if (!arg || !*arg) {
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of these lores:\n\r\n\r");
            strcat(buffer, buf);
            PrintSkills(ch, -1, loreskills, loreskillCount, buffer);
            page_string(ch->desc, buffer, 1);
        } else if( !LearnSkill(ch, loreskills, loreskillCount, arg, 
                               GetMaxLevel(ch), "The loremaster", 
                               GetMaxLevel(ch) * 100) ) {
            send_to_char("$c0013[$c0015The loremaster$c0013] tells you '"
                         "I do not know of that skill!'\n\r", ch);
        }
        return (TRUE);
    }
    return (FALSE);
}

int hunter(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    char            buf[256];
    static char    *n_skills[] = {
        "hunt",
        "find traps",
        "remove traps",
        "value item",
        "find food",            /* 5 */
        "find water",           /* 6 */
        "\n",
    };
    int             percent = 0,
                    number = 0;
    int             charge,
                    sk_num;

    if (!AWAKE(ch)) {
        return (FALSE);
    }
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
        return (FALSE);
    }

    if (!ch->skills) {
        return (FALSE);
    }
    if (check_soundproof(ch)) {
        return (FALSE);
    }

    arg = skip_spaces(arg);

    if (cmd == 164 || cmd == 170) {
        if (!arg) {
            sprintf(buf, " hunt           :  %s\n\r",
                    how_good(ch->skills[SKILL_HUNT].learned));
            send_to_char(buf, ch);
            sprintf(buf, " find traps     :  %s\n\r",
                    how_good(ch->skills[SKILL_FIND_TRAP].learned));
            send_to_char(buf, ch);
            sprintf(buf, " remove traps   :  %s\n\r",
                    how_good(ch->skills[SKILL_REMOVE_TRAP].learned));
            send_to_char(buf, ch);
            sprintf(buf, " value item     :  %s\n\r",
                    how_good(ch->skills[SKILL_EVALUATE].learned));
            send_to_char(buf, ch);
            sprintf(buf, " find food      :  %s\n\r",
                    how_good(ch->skills[SKILL_FIND_FOOD].learned));
            send_to_char(buf, ch);
            sprintf(buf, " find water     :  %s\n\r",
                    how_good(ch->skills[SKILL_FIND_WATER].learned));
            send_to_char(buf, ch);

            return (TRUE);
        }

        number = old_search_block(arg, 0, strlen(arg), n_skills, FALSE);
        send_to_char("The hunter says ", ch);
        if (number == -1) {
            send_to_char("'I do not know of this skill.'\n\r", ch);
            return (TRUE);
        }

        charge = GetMaxLevel(ch) * 100;
        switch (number) {
        case 0:
        case 1:
            sk_num = SKILL_HUNT;
            break;
        case 2:
            sk_num = SKILL_FIND_TRAP;
            break;
        case 3:
            sk_num = SKILL_REMOVE_TRAP;
            break;
        case 4:
            sk_num = SKILL_EVALUATE;
            break;
        case 5:
            sk_num = SKILL_FIND_FOOD;
            break;
        case 6:
            sk_num = SKILL_FIND_WATER;
            break;
        default:
            Log("Strangeness in hunter (%d)", number);
            send_to_char("'Ack!  I feel faint!'\n\r", ch);
            return (FALSE);
        }

        if (!HasClass(ch, CLASS_THIEF) &&
            (sk_num == SKILL_FIND_TRAP || sk_num == SKILL_REMOVE_TRAP ||
             sk_num == SKILL_EVALUATE)) {
            send_to_char("'You're not the sort I'll teach trap or value "
                         "skills to.'\n\r", ch);
            return (TRUE);
        }

        if (sk_num == SKILL_HUNT && !HasClass(ch, CLASS_THIEF) &&
            !HasClass(ch, CLASS_BARBARIAN)) {
            send_to_char("'You're not the sort I'll teach track to.'\n\r", ch);
            return (TRUE);
        }

        if (GET_GOLD(ch) < charge) {
            send_to_char("'Ah, but you do not have enough money to pay.'\n\r",
                         ch);
            return (TRUE);
        }

        if (ch->skills[sk_num].learned >= 95) {
            send_to_char("'You are a master of this art, I can teach you no "
                         "more.'\n\r", ch);
            return (TRUE);
        }

        if (ch->specials.spells_to_learn <= 0) {
            send_to_char("'You must first earn more practices you already "
                         "have.\n\r", ch);
            return (TRUE);
        }

        GET_GOLD(ch) -= charge;
        send_to_char("'We will now begin.'\n\r", ch);
        ch->specials.spells_to_learn--;

        percent = ch->skills[sk_num].learned + int_app[(int)GET_INT(ch)].learn;

        if (!IS_SET(ch->skills[sk_num].flags, SKILL_KNOWN)) {
            SET_BIT(ch->skills[sk_num].flags, SKILL_KNOWN);
        }

        ch->skills[sk_num].learned = MIN(95, percent);

        if (ch->skills[sk_num].learned >= 95) {
            send_to_char("'You are now a master of this art.'\n\r", ch);
            return (TRUE);
        }
    }

    return (FALSE);
}

int archer_instructor(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    /*
     * 170->Practice,164->Practise, 243->gain
     */
    if (cmd == 164 || cmd == 170) {
        if (!HasClass(ch, CLASS_WARRIOR | CLASS_BARBARIAN | CLASS_RANGER |
                          CLASS_PALADIN)) {
            send_to_char("$c0013[$c0015The archer instructor$c0013] tells you"
                         " 'I'm not teaching the likes of you, get lost!'\n\r",
                         ch);
            return (TRUE);
        }

        if (!arg || !*arg) {
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice this skill:\n\r\n\r");
            strcat(buffer, buf);

            PrintSkills(ch, 50, archerskills, archerskillCount, buffer);
            page_string(ch->desc, buffer, 1);
        } else if( !LearnSkill(ch, archerskills, archerskillCount, arg,
                               GetMaxLevel(ch), "The archer instructor",
                               GetMaxLevel(ch) * 100) ) {
            send_to_char("$c0013[$c0015The archer instructor$c0013] tells you"
                         " 'I do not know of that skill!'\n\r", ch);
        }
        return (TRUE);
    }
    return (FALSE);
}

int monk_master(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];
    struct char_data *guildmaster;

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    /*
     * 243->gain
     */
    if (cmd == 243) {
        if (!HasClass(ch, CLASS_MONK)) {
            send_to_char("$c0013[$c0015The Monk Guildmaster$c0013] tells you"
                         " 'You're not a monk.'\n\r", ch);
            return (TRUE);
        }

        if (!mob) {
            guildmaster = FindMobInRoomWithFunction(ch->in_room, monk_master);
        } else {
            guildmaster = mob;
        }

        if (GET_LEVEL(ch, MONK_LEVEL_IND) > GetMaxLevel(guildmaster)) {
            send_to_char("$c0013[$c0015The Monk Guildmaster$c0013] tells you"
                         " 'You must learn from another, I can no longer train"
                         " you.'\n\r", ch);
            return (TRUE);
        }

        if( GET_EXP(ch) <
            classes[MONK_LEVEL_IND].
                levels[GET_LEVEL(ch, MONK_LEVEL_IND) + 1].exp) {
            send_to_char("You're not ready to gain yet!\n\r", ch);
            return (FALSE);
        }

        if (GET_LEVEL(ch, MONK_LEVEL_IND) <= 9) {
            GainLevel(ch, MONK_LEVEL_IND);
        } else {
            send_to_char("You must fight another monk for this title.\n\r", ch);
        }
        return (TRUE);
    } else if (cmd == 164 || cmd == 170 || cmd == 582) {
        if (HasClass(ch, CLASS_MONK)) {
            if (!arg || !*arg) {
                sprintf(buffer, "You have got %d practice sessions "
                                "left.\n\r\n\r", ch->specials.spells_to_learn);
                sprintf(buf, "You can practice any of these skills:\n\r\n\r");
                strcat(buffer, buf);
                PrintSkills(ch, GET_LEVEL(ch, MONK_LEVEL_IND),
                            classes[MONK_LEVEL_IND].skills,
                            classes[MONK_LEVEL_IND].skillCount, buffer);

                if (ch->specials.remortclass == MONK_LEVEL_IND + 1) {
                    sprintf(buf, "\n\rSince you picked monk as your main "
                                 "class, you get these bonus skills:\n\r\n\r");
                    strcat(buffer, buf);
                    PrintSkills(ch, GET_LEVEL(ch, MONK_LEVEL_IND),
                                classes[MONK_LEVEL_IND].mainskills,
                                classes[MONK_LEVEL_IND].mainskillCount, buffer);
                }
                page_string(ch->desc, buffer, 1);
            } else {
                if( LearnSkill(ch, classes[MONK_LEVEL_IND].skills, 
                               classes[MONK_LEVEL_IND].skillCount, arg,
                               GET_LEVEL(ch, MONK_LEVEL_IND),
                               "The Monk Guildmaster", 0) ) {
                    return( TRUE );
                }

                if (ch->specials.remortclass == MONK_LEVEL_IND + 1 &&
                    !LearnSkill(ch, classes[MONK_LEVEL_IND].mainskills,
                                classes[MONK_LEVEL_IND].mainskillCount, arg,
                                GET_LEVEL(ch, MONK_LEVEL_IND),
                                "The Monk Guildmaster", 0) ) {
                    send_to_char("$c0013[$c0015The Monk Guildmaster$c0013] "
                                 "tells you 'I do not know of that skill!'\n\r",
                                 ch);
                }
            }
            return (TRUE);
        } else if (HasClass(ch, CLASS_WARRIOR)) {
            if (!arg || !*arg) {
                sprintf(buffer, "You have got %d practice sessions "
                                "left.\n\r\n\r", ch->specials.spells_to_learn);
                sprintf(buf, "You can practice these skills:\n\r\n\r");
                strcat(buffer, buf);
                PrintSkills(ch, GET_LEVEL(ch, WARRIOR_LEVEL_IND),
                            warmonkskills, warmonkskillCount, buffer);
                page_string(ch->desc, buffer, 1);
            } else {
                if( LearnSkill(ch, warmonkskills, warmonkskillCount, arg,
                               GET_LEVEL(ch, WARRIOR_LEVEL_IND),
                               "The Monk Guildmaster", 0) ) {
                    return (TRUE);
                }
                send_to_char("$c0013[$c0015The Monk Guildmaster$c0013] "
                             "tells you 'I do not know of that skill!'\n\r",
                             ch);
            }
            return (TRUE);
        } else {
            send_to_char("$c0013[$c0015The Monk Guildmaster$c0013] tells you '"
                         "I don't teach the likes of you!'\n\r", ch);
            return (TRUE);
        }
    }
    return (FALSE);
}

int DruidGuildMaster(struct char_data *ch, int cmd, char *arg,
                     struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_DRUID,
                                DRUID_LEVEL_IND);
}


int barbarian_guildmaster(struct char_data *ch, int cmd, char *arg,
                          struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_BARBARIAN,
                               BARBARIAN_LEVEL_IND);
}

int RangerGuildmaster(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_RANGER,
                                RANGER_LEVEL_IND);
}

int PsiGuildmaster(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_PSI,
                                PSI_LEVEL_IND);
}

int PaladinGuildmaster(struct char_data *ch, int cmd, char *arg,
                       struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_PALADIN,
                                PALADIN_LEVEL_IND);
}

int mage_specialist_guildmaster(struct char_data *ch, int cmd, char *arg,
                                struct char_data *mob, int type)
{
    char            buf[256];
    int             i,
                    index,
                    number = 0;
    struct char_data *guildmaster;

    if (!AWAKE(ch)) {
        return (FALSE);
    }
    if (!cmd) {
        if (ch->specials.fighting) {
            return (magic_user(ch, cmd, arg, ch, 0));
        }
        return (FALSE);
    }

    if (!ch->skills) {
        return (FALSE);
    }
    if (check_soundproof(ch)) {
        return (FALSE);
    }
    guildmaster = FindMobInRoomWithFunction(ch->in_room,
                                            mage_specialist_guildmaster);

    if (!guildmaster) {
        return (FALSE);
    }

    arg = skip_spaces(arg);

    if ((cmd == 164) || (cmd == 170) || cmd == 243 || cmd == 582) {
        if (!HasClass(ch, CLASS_MAGIC_USER)) {
            do_say(guildmaster, "I do not teach heathens!", 0);
            return (TRUE);
        }

        if (HasFightingClass(ch) == TRUE) {
            do_say(guildmaster, "Your warrior instincts are rather scary.. "
                                "Begone...", 0);
            return (FALSE);
        }

        if (IS_NPC(ch)) {
            do_say(guildmaster, "What do I look like? An animal trainer?", 0);
            return (FALSE);
        }

        if (!arg) {
            sprintf(buf, "You have got %d practice sessions left.\n\r",
                    ch->specials.spells_to_learn);
            send_to_char(buf, ch);
            send_to_char("You can practise any of these spells:\n\r", ch);

            for (i = 0; *spells[i] != '\n'; i++) {
                index = spell_index[i + 1];
                if( index == -1 ) {
                    continue;
                }

                if (spell_info[index].spell_pointer &&
                    spell_info[index].min_level_magic <=
                    GET_LEVEL(ch, MAGE_LEVEL_IND) &&
                    spell_info[i + 1].min_level_magic <=
                     GetMaxLevel(guildmaster) - 10) {

                    sprintf(buf, "[%d] %-15s %s \n\r",
                            spell_info[index].min_level_magic,
                            spells[i], how_good(ch->skills[i + 1].learned));
                    send_to_char(buf, ch);
                }
            }
            return (TRUE);
        }

        arg = skip_spaces(arg);
        if( arg ) {
            number = old_search_block(arg, 0, strlen(arg), spells, FALSE);
        } else {
            number = -1;
        }
        index = spell_index[number];
        if (number == -1 || index == -1) {
            do_say(guildmaster, "You do not know of this spell.", 0);
            return (TRUE);
        }

        if (GET_LEVEL(ch, MAGE_LEVEL_IND) <
            spell_info[index].min_level_magic) {
            do_say(guildmaster, "You do not know of this spell.", 0);
            return (TRUE);
        }

        if (GetMaxLevel(guildmaster) - 10 <
            spell_info[index].min_level_magic) {
            do_say(guildmaster, "I don't know of this spell.", 0);
            return (TRUE);
        }

        if (GET_GOLD(ch) < SPELL_SPECIAL_COST) {
            do_say(guildmaster, "Ah, but you do not have enough money to pay.",
                   0);
            return (TRUE);
        }

        if (ch->specials.spells_to_learn < 4) {
            do_say(guildmaster, "You must first use the knowledge you already "
                                "have.", 0);
            return (TRUE);
        }

        if (IsSpecialized(ch->skills[number].special)) {
            do_say(guildmaster, "You are already proficient in this spell!", 0);
            return (TRUE);
        }

        if (HowManySpecials(ch) > MAX_SPECIALS(ch)) {
            do_say(guildmaster, "You are already specialized in several "
                                "skills.", 0);
            return (TRUE);
        }

        if (ch->skills[number].learned < 95) {
            do_say(guildmaster, "You must fully learn this spell first.", 0);
            return (TRUE);
        }

        do_say(guildmaster, "Here is how you do that...", 0);
        ch->specials.spells_to_learn -= 4;
        GET_GOLD(ch) -= SPELL_SPECIAL_COST;
        SET_BIT(ch->skills[number].special, SKILL_SPECIALIZED);
        return (TRUE);
    }
    return (magic_user(ch, cmd, arg, ch, 0));
}

int cleric_specialist_guildmaster(struct char_data *ch, int cmd, char *arg,
                                  struct char_data *mob, int type)
{
    char            buf[256];
    int             i,
                    index,
                    number = 0;
    struct char_data *guildmaster;

    if (!AWAKE(ch)) {
        return (FALSE);
    }
    if (!cmd) {
        if (ch->specials.fighting) {
            return (cleric(ch, cmd, arg, ch, 0));
        }
        return (FALSE);
    }

    if (!ch->skills) {
        return (FALSE);
    }
    if (check_soundproof(ch)) {
        return (FALSE);
    }
    guildmaster = FindMobInRoomWithFunction(ch->in_room,
                                            cleric_specialist_guildmaster);

    if (!guildmaster) {
        return (FALSE);
    }

    arg = skip_spaces(arg);

    if ((cmd == 164) || (cmd == 170) || cmd == 243 || cmd == 582) {
        if (!HasClass(ch, CLASS_CLERIC)) {
            do_say(guildmaster, "I do not teach heathens!", 0);
            return (TRUE);
        }

        if (HasFightingClass(ch) == TRUE) {
            do_say(guildmaster, "Your warrior instincts are rather scary.. "
                                "Begone...", 0);
            return (FALSE);
        }

        if (IS_NPC(ch)) {
            do_say(guildmaster, "What do I look like? An animal trainer?", 0);
            return (FALSE);
        }

        if (!arg) {
            sprintf(buf, "You have got %d practice sessions left.\n\r",
                    ch->specials.spells_to_learn);
            send_to_char(buf, ch);
            send_to_char("You can practise any of these spells:\n\r", ch);

            for (i = 0; *spells[i] != '\n'; i++) {
                index = spell_index[i + 1];
                if( index == -1 ) {
                    continue;
                }

                if (spell_info[index].spell_pointer &&
                    spell_info[index].min_level_cleric <=
                    GET_LEVEL(ch, CLERIC_LEVEL_IND) &&
                    spell_info[index].min_level_cleric <=
                     GetMaxLevel(guildmaster) - 10) {
                    sprintf(buf, "[%d] %-15s %s \n\r",
                            spell_info[index].min_level_cleric,
                            spells[i], how_good(ch->skills[i + 1].learned));
                    send_to_char(buf, ch);
                }
            }

            return (TRUE);
        }

        arg = skip_spaces(arg);
        if( arg ) {
            number = old_search_block(arg, 0, strlen(arg), spells, FALSE);
        } else {
            number = -1;
        }
        index = spell_index[number];
        if (number == -1 || index == -1) {
            do_say(guildmaster, "You do not know of this spell.", 0);
            return (TRUE);
        }

        if (GET_LEVEL(ch, CLERIC_LEVEL_IND) <
            spell_info[index].min_level_cleric) {
            do_say(guildmaster, "You do not know of this spell.", 0);
            return (TRUE);
        }

        if (GetMaxLevel(guildmaster) - 10 <
            spell_info[index].min_level_cleric) {
            do_say(guildmaster, "I don't know of this spell.", 0);
            return (TRUE);
        }

        if (GET_GOLD(ch) < SPELL_SPECIAL_COST) {
            do_say(guildmaster, "Ah, but you do not have enough money to pay.",
                   0);
            return (TRUE);
        }

        if (ch->specials.spells_to_learn < 4) {
            do_say(guildmaster, "You do not seem to be able to practice at the"
                                " moment.", 0);
            return (TRUE);
        }

        if (IsSpecialized(ch->skills[number].special)) {
            do_say(guildmaster, "You are already proficient in this spell!", 0);
            return (TRUE);
        }

        if (HowManySpecials(ch) > MAX_SPECIALS(ch)) {
            do_say(guildmaster, "You are already specialized in several "
                                "skills.", 0);
            return (TRUE);
        }

        if (ch->skills[number].learned < 95) {
            do_say(guildmaster, "You must fully learn this spell first.", 0);
            return (TRUE);
        }

        do_say(guildmaster, "Here is how you do that...", 0);
        ch->specials.spells_to_learn -= 4;
        GET_GOLD(ch) -= SPELL_SPECIAL_COST;
        SET_BIT(ch->skills[number].special, SKILL_SPECIALIZED);
        return (TRUE);
    }
    return (cleric(ch, cmd, arg, ch, 0));
}


int ninja_master(struct char_data *ch, int cmd, char *arg,
                 struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    /*
     * 170->Practice,164->Practise,
     */
    if (cmd == 164 || cmd == 170 || cmd == 582) {
        if (!arg || !*arg) {
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of these skills:\n\r\n\r");
            strcat(buffer, buf);

            if (HasClass(ch, CLASS_WARRIOR)) {
                PrintSkills(ch, -1, warninjaskills, warninjaskillCount, buffer);
            }

            if (HasClass(ch, CLASS_THIEF)) {
                PrintSkills(ch, -1, thfninjaskills, thfninjaskillCount, buffer);
            }

            PrintSkills(ch, -1, allninjaskills, allninjaskillCount, buffer);
            page_string(ch->desc, buffer, 1);
        } else {
            if (HasClass(ch, CLASS_WARRIOR) &&
                LearnSkill(ch, warninjaskills, warninjaskillCount, arg, 
                           GetMaxLevel(ch), "The Ninja Master", 0) ) {
                return( TRUE );
            }

            if (HasClass(ch, CLASS_THIEF) &&
                LearnSkill(ch, thfninjaskills, thfninjaskillCount, arg,
                           GetMaxLevel(ch), "The Ninja Master", 0) ) {
                return( TRUE );
            }

            if (LearnSkill(ch, allninjaskills, allninjaskillCount, arg,
                           GetMaxLevel(ch), "The Ninja Master", 0) ) {
                return( TRUE );
            }

            send_to_char("$c0013[$c0015The Ninja Master$c0013] tells you '"
                         "I do not know of that skill!'\n\r", ch);
        }
        return (TRUE);
    }
    return (FALSE);
}


int TrainingGuild(struct char_data *ch, int cmd, char *arg,
                  struct char_data *mob, int type)
{
    char            name[32];

    int             x = 0,
                    stat = 0,
                    cost = 3;

    struct skillset traininglist[] = {
        {"constitution", 4, 0},
        {"strength", 5, 0},
        {"dexterity", 6, 0},
        {"charisma", 7, 0},
        {"intelligence", 8, 0},
        {"wisdom", 9, 0},
        {"None", -1, -1}
    };

    traininglist[0].maxlearn = GET_RCON(ch) - 3;
    traininglist[1].maxlearn = GET_RSTR(ch) - 3;
    traininglist[2].maxlearn = GET_RDEX(ch) - 3;
    traininglist[3].maxlearn = GET_RCHR(ch) - 3;
    traininglist[4].maxlearn = GET_RINT(ch) - 3;
    traininglist[5].maxlearn = GET_RWIS(ch) - 3;

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    if (cmd != 582 && cmd != 59) {
        return (FALSE);
    }
    if (cmd == 582 && (!arg || !*arg)) {
        /*
         * list if no argument
         */
        oldSendOutput(ch, "$c000B%-15s     %-3s\n\r------------------------\n\r",
                  "Stat", "Cost");
        while (traininglist[x].level != -1) {
            oldSendOutput(ch, "$c000W %-15s     %-3d\n\r",
                      traininglist[x].name,
                      traininglist[x].level < 3 ? 3 : traininglist[x].level);
            x++;
        }
        oldSendOutput(ch, "$c000B------------------------\n\r");
        return (TRUE);
    }

    if (cmd == 582) {
        /*
         * train
         */
        while (traininglist[x].level != -1) {
            if (is_abbrev(arg, traininglist[x].name)) {
                cost = traininglist[x].level;
                if (cost < 3) {
                    cost = 3;
                }
                stat = x + 1;
                if (GET_PRAC(ch) - cost < 0) {
                    oldSendOutput(ch, "$c000P%s tells you 'You don't have enough "
                                  "practice sessions to learn %s.'\n\r",
                              mob->player.short_descr, traininglist[x].name);
                    return (TRUE);
                }
                break;
            }
            x++;
        }

        switch (stat) {
        case 1:
            if (GET_RCON(ch) >= 17) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              " further.'\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RCON(ch) >= MaxConForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You allready have the "
                              "maximum con for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_RCON(ch) = GET_RCON(ch) + 1;
                GET_CON(ch) = GET_CON(ch) + 1;

                GET_PRAC(ch) -= cost;
                oldSendOutput(ch, "$c000P%s tells you 'Lets train your "
                              "constitution!!!'\n\r$c000wYou heed his advice "
                              "and go for a jog around the room.(+1 Con)\n\r",
                          mob->player.short_descr);
            }
            break;
        case 2:
            if (affected_by_spell(ch, SPELL_STRENGTH)) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s "
                              "while it is magically enhanced.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (affected_by_spell(ch, SPELL_HOLY_STRENGTH)) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s "
                              "while it is magically enhanced.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RSTR(ch) >= 18) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              "further.'\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RSTR(ch) >= MaxStrForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You allready have the "
                              "maximum %s for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_PRAC(ch) -= cost;
                GET_RSTR(ch) = GET_RSTR(ch) + 1;
                GET_STR(ch) = GET_STR(ch) + 1;

                oldSendOutput(ch, "$c000P%s tells you 'Lets train your "
                              "strength!!!'\n\r$c000wYou start lifting some "
                              "weights.  You feel stronger!!!(+1 Str)'\n\r",
                          mob->player.short_descr);
            }
            break;
        case 3:
            if (GET_RDEX(ch) >= 17) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              "further.'\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RDEX(ch) >= MaxDexForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You already have the "
                              "maximum %s for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_PRAC(ch) -= cost;
                GET_RDEX(ch) = GET_RDEX(ch) + 1;;
                GET_DEX(ch) = GET_DEX(ch) + 1;
                oldSendOutput(ch, "$c000P%s tells you 'Lets train your dex!!!\n\r"
                              "$c000w%s shows you some stretches.  You mimic "
                              "them!!! (+1 Dex)\n\r",
                          mob->player.short_descr, mob->player.short_descr);
            }
            break;
        case 4:
            if (GET_RCHR(ch) >= 17) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              "further.'\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RCHR(ch) >= MaxChrForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You allready have the "
                              "maximum %s for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_PRAC(ch) -= cost;
                GET_RCHR(ch) = GET_RCHR(ch) + 1;
                GET_CHR(ch) = GET_CHR(ch) + 1;
                oldSendOutput(ch, "$c000P%s tells you 'Lets train your charisma!!!'"
                              "\n\r$c000w%s gives you some lessons in "
                              "manners!!(+1 Chr)\n\r",
                          mob->player.short_descr, mob->player.short_descr);
            }
            break;
        case 5:
            if (GET_RINT(ch) >= 17) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              "further.\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RINT(ch) >= MaxIntForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You allready have the "
                              "maximum %s for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_PRAC(ch) -= cost;
                GET_RINT(ch) = GET_RINT(ch) + 1;
                GET_INT(ch) = GET_INT(ch) + 1;
                oldSendOutput(ch, "$c000P%s tells you 'Lets train your "
                              "intelligence!!!\n\r$c000w%s gives you a strange"
                              " old book to read. You read it!!(+1 Int)\n\r",
                          mob->player.short_descr, mob->player.short_descr);
            }
            break;
        case 6:
            if (GET_RWIS(ch) >= 17) {
                oldSendOutput(ch, "$c000P%s tells you 'I cannot train your %s any "
                              "further.'\n\r", mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            }

            if (GET_RWIS(ch) >= MaxWisForRace(ch)) {
                oldSendOutput(ch, "$c000P%s tells you 'You allready have the "
                              "maximum %s for your race.'\n\r",
                          mob->player.short_descr,
                          traininglist[stat - 1].name, name);
                break;
            } else {
                GET_PRAC(ch) -= cost;
                GET_RWIS(ch) = GET_RWIS(ch) + 1;
                GET_WIS(ch) = GET_WIS(ch) + 1;
                oldSendOutput(ch, "$c000P%s tells you 'Lets train your "
                              "wisdom!!!\n\r$c000w%s sits down and brings out "
                              "the old chess board!!\n\rYou and him have a "
                              "chat and play a few games.(+1 Wis)'\n\r",
                          mob->player.short_descr, mob->player.short_descr);
            }
            break;
        default:
            oldSendOutput(ch, "$c000P%s tells you 'I'm not quite sure how to train"
                          " that.'\n\r", mob->player.short_descr);
            break;
        }
    }
    return( TRUE );
}

int WeaponsMaster(struct char_data *ch, int cmd, char *arg,
                  struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch))
        return (FALSE);

    /*
     * 170->Practice,164->Practise, 243->gain
     */
    if (cmd == 164 || cmd == 170) {

        if (!OnlyClass(ch, CLASS_WARRIOR)) {
            send_to_char("Only single class warriors can learn weapon styles "
                         "proficiently.", ch);
            return (FALSE);
        }

        if (!arg || !*arg) {
            /*
             * practice||practise, without argument
             */
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of these weapon "
                         "styles:\n\r\n\r");
            strcat(buffer, buf);
            PrintSkills(ch, 50, weaponskills, weaponskillCount, buffer);
            page_string(ch->desc, buffer, 1);
            return (TRUE);
        }

        if( !LearnSkill(ch, weaponskills, weaponskillCount, arg, 50,
                        "Weapon Master", 0 ) ) {
            send_to_char("$c0013[$c0015Weapon Master$c0013] tells you '"
                         "I do not know of that skill!'\n\r", ch);
        }
        return (TRUE);
    }
    return (FALSE);
}

int NecromancerGuildMaster(struct char_data *ch, int cmd, char *arg,
                           struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_NECROMANCER,
                                NECROMANCER_LEVEL_IND);
}

int generic_guildmaster(struct char_data *ch, int cmd, char *arg,
                        struct char_data *mob, int type)
{
    struct char_data *guildmaster = NULL,
                   *j,
                   *next;
    struct room_data *rp;

    if (!AWAKE(ch) || IS_NPC(ch) || !ch->in_room ||
        !(rp = real_roomp(ch->in_room))) {
        return (FALSE);
    }

    for (j = rp->people; j; j = next) {
        next = j->next_in_room;
        if (j->specials.proc == PROC_GUILDMASTER) {
            guildmaster = j;
            break;
        }
    }

    if (!guildmaster) {
        Log("weirdness in generic_guildmaster, assigned but not found");
        return (FALSE);
    }

    if (!IS_NPC(guildmaster)) {
        Log("weirdness in guildmaster, is not a mob");
        return (FALSE);
    }

    if (cmd != 164 && cmd != 170 && cmd != 243)
        return (FALSE);

    /*
     * let's see which GM we got here
     */
    if (IS_SET(guildmaster->specials.act_class, ACT_MAGIC_USER)) {
        MageGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_CLERIC)) {
        ClericGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_WARRIOR)) {
        WarriorGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_THIEF)) {
        ThiefGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_DRUID)) {
        DruidGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_MONK)) {
        monk_master(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_BARBARIAN)) {
        barbarian_guildmaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_PALADIN)) {
        PaladinGuildmaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_RANGER)) {
        RangerGuildmaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_PSI)) {
        PsiGuildmaster(ch, cmd, 0, guildmaster, 0);
    } else if (IS_SET(guildmaster->specials.act_class, ACT_NECROMANCER)) {
        NecromancerGuildMaster(ch, cmd, 0, guildmaster, 0);
    } else {
        Log("guildmaster proc attached to mobile without a class, autoset to"
            " warrior");
        SET_BIT(guildmaster->specials.act_class, ACT_WARRIOR);
    }
    return (TRUE);
}


int remort_guild(struct char_data *ch, int cmd, char *arg,
                 struct char_data *mob, int type)
{
    int             classcombos[] = {
        CLASS_MAGIC_USER + CLASS_CLERIC + CLASS_WARRIOR + CLASS_THIEF,
        CLASS_DRUID + CLASS_RANGER + CLASS_WARRIOR,
        CLASS_SORCERER + CLASS_CLERIC + CLASS_WARRIOR + CLASS_THIEF,
        CLASS_RANGER + CLASS_DRUID,
        CLASS_PSI + CLASS_WARRIOR + CLASS_THIEF,
        CLASS_NECROMANCER + CLASS_WARRIOR + CLASS_THIEF,
        -1
    };

    int             x,
                    hasclass = 0,
                    num = 0,
                    choose = -1,
                    avg = 0;

    char           *classname;

    if (cmd != 605) {
        return (FALSE);
    }

    arg = get_argument(arg, &classname);
    if (!classname) {
        do_mobTell2(ch, mob, "A class you must choose!");
        return (TRUE);
    }

    /*
     * lets count what classes ch has now
     */

    for (x = 0; x < MAX_CLASS; x++) {
        if (HasClass(ch, pc_num_class(x))) {
            hasclass += pc_num_class(x);
            num += 1;
            avg += ch->player.level[x];
        }
        if (is_abbrev(classname, classes[x].name)) {
            choose = x;
#if 0
            oldSendOutput(ch,"You choose class# x%d as a new class.pc%d.\n\r"
                      ,x,pc_num_class(x));
#endif
        }
    }

    if (avg / num < 50 || GET_LEADERSHIP_EXP(ch) < 50000000) {
        /*
         * Average level is 50?? they maxxed out??
         */
        do_mobTell2(ch, mob, "You don't look strong enough or worthy enough "
                             "to be in my presences.");
        return (TRUE);
    }

    /*
     * didn't find a class argument
     */
    if (choose == -1) {
        do_mobTell2(ch, mob, "That profession is unknown to me!");
        return (TRUE);
    }

    /*
     * Check to see if they have that class
     */
    if (HasClass(ch, pc_num_class(choose))) {
        do_mobTell2(ch, mob, "You already know enough about that class!");
        return (TRUE);
    }

    /*
     * See if they have too many classes
     */
    if (num > 2) {
        do_mobTell2(ch, mob, "I'm afraid you already have too many "
                             "professions!");
        return (TRUE);
    }

    hasclass += pc_num_class(choose);

    x = 0;
    while (classcombos[x] != -1) {
        if (IS_SET(classcombos[x], hasclass)) {
            do_mobTell2(ch, mob, "You may now know the art of this class!");
            oldSendOutput(ch, "You just obtained a new class!!! %d",
                      pc_num_class(choose));
            ch->player.class = hasclass;
            do_restore(ch, GET_NAME(ch), 0);
            ch->specials.remortclass = choose;

            ch->points.max_hit = ch->points.max_hit / GET_CON(ch);

            /*
             * reset the char
             */
            for (x = 0; x < MAX_CLASS; x++) {
                if (HasClass(ch, pc_num_class(x))) {
                    /*
                     * set all classes to level 1
                     */
                    ch->player.level[x] = 1;
                } else
                    ch->player.level[x] = 0;
            }
            GET_EXP(ch) = 1;
            GET_LEADERSHIP_EXP(ch) = 1;
            return (TRUE);
        }
        x++;
    }

    /*
     * No class combos found
     */
    do_mobTell2(ch, mob, "You can't multi-class with that class!");
    return (TRUE);
}


char           *how_good(int percent)
{
    static char     buf[256];

    if (percent == 0) {
        strcpy(buf, " (not learned)");
    } else if (percent <= 10) {
        strcpy(buf, " (awful)");
    } else if (percent <= 20) {
        strcpy(buf, " (bad)");
    } else if (percent <= 40) {
        strcpy(buf, " (poor)");
    } else if (percent <= 55) {
        strcpy(buf, " (average)");
    } else if (percent <= 70) {
        strcpy(buf, " (fair)");
    } else if (percent <= 80) {
        strcpy(buf, " (good)");
    } else if (percent <= 85) {
        strcpy(buf, " (very good)");
    } else {
        strcpy(buf, " (Superb)");
    }
    return (buf);
}

int GainLevel(struct char_data *ch, int class)
{
    if (GET_EXP(ch) >= classes[class].levels[GET_LEVEL(ch, class) + 1].exp) {
        if (GET_LEVEL(ch, class) < races[GET_RACE(ch)].racialMax[class]) {
            if (GET_LEVEL(ch, class) < 50) {
                send_to_char("You raise a level!\n\r", ch);
                advance_level(ch, class);
                set_title(ch);
                return (TRUE);
            } else {
                send_to_char("You cannot join the ranks of the immortals "
                             "without the help of a Creator!\n\r", ch);
            }
        } else {
            send_to_char("You are unable to advance further in this class\n\r",
                         ch);
        }
    } else {
        send_to_char("You haven't got enough experience!\n\r", ch);
    }
    return (FALSE);
}


int MageGuildMaster(struct char_data *ch, int cmd, char *arg,
                    struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_MAGIC_USER,
                                MAGE_LEVEL_IND);
}

int SorcGuildMaster(struct char_data *ch, int cmd, char *arg,
                    struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch))
        return (FALSE);

    /*
     * 170->Practice,164->Practise, 243->gain
     */
    if (cmd == 164 || cmd == 170 || cmd == 243 || cmd == 582) {

        if (!HasClass(ch, CLASS_SORCERER)) {
            send_to_char
                ("$c0013[$c0015The Sorcerer Guildmaster$c0013] tells you"
                 " 'You're not a sorcerer.'\n\r", ch);
            return (TRUE);
        }
        /*
         * gain
         */
        if (cmd == 243) {
            if ( GET_EXP(ch) <
                classes[SORCERER_LEVEL_IND].
                    levels[GET_LEVEL(ch, SORCERER_LEVEL_IND) + 1].exp) {
                send_to_char("You're not ready to gain yet!", ch);
                return (FALSE);
            } else {
                GainLevel(ch, SORCERER_LEVEL_IND);
                return (TRUE);
            }
        }

        if (!arg || !*arg) {
            /*
             * practice||practise, without argument
             */
            sprintf(buffer, "You have got %d practice sessions left.\n\r\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of these spells:\n\r\n\r");
            strcat(buffer, buf);
            PrintSkills(ch, GET_LEVEL(ch, SORCERER_LEVEL_IND),
                        classes[SORCERER_LEVEL_IND].skills,
                        classes[SORCERER_LEVEL_IND].skillCount, buffer);

            if (ch->specials.remortclass == SORCERER_LEVEL_IND + 1) {
                sprintf(buf, "\n\rSince you picked sorcerer as your main class,"
                             " you get these bonus skills:\n\r\n\r");
                strcat(buffer, buf);
                PrintSkills(ch, GET_LEVEL(ch, SORCERER_LEVEL_IND),
                            classes[SORCERER_LEVEL_IND].mainskills,
                            classes[SORCERER_LEVEL_IND].mainskillCount, buffer);
            }
            page_string(ch->desc, buffer, 1);
            return (TRUE);
        }

        if( LearnSkill(ch, classes[SORCERER_LEVEL_IND].skills, 
                       classes[SORCERER_LEVEL_IND].skillCount, arg,
                       GET_LEVEL(ch, SORCERER_LEVEL_IND),
                       "The Sorcerer Guildmaster", 0 ) ) {
            return( TRUE );
        }

        if (ch->specials.remortclass == SORCERER_LEVEL_IND + 1 &&
            LearnSkill(ch, classes[SORCERER_LEVEL_IND].mainskills,
                       classes[SORCERER_LEVEL_IND].mainskillCount, arg,
                       GET_LEVEL(ch, SORCERER_LEVEL_IND),
                       "The Sorcerer Guildmaster", 0 ) ) {
            return( TRUE );
        }

        send_to_char("$c0013[$c0015The Sorcerer Guildmaster$c0013] tells you '"
                     "I do not know of that skill!'\n\r", ch);
        return (TRUE);
    }
    return (FALSE);
}

int ClericGuildMaster(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_CLERIC,
                                CLERIC_LEVEL_IND);
}

int ThiefGuildMaster(struct char_data *ch, int cmd, char *arg,
                     struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_THIEF,
                                THIEF_LEVEL_IND);
}

int WarriorGuildMaster(struct char_data *ch, int cmd, char *arg,
                       struct char_data *mob, int type)
{
    return guildmaster_skeleton(ch, cmd, arg, mob, type, CLASS_WARRIOR,
                                WARRIOR_LEVEL_IND);
}

int FightingGuildMaster(struct char_data *ch, int cmd, char *arg,
                        struct char_data *mob, int type)
{
    char            buf[256],
                    buffer[MAX_STRING_LENGTH];

    if (!AWAKE(ch) || IS_NPC(ch)) {
        return (FALSE);
    }
    if (cmd == 164 || cmd == 170 || cmd == 582) {
        if (GetMaxLevel(ch) < 10) {
            send_to_char("$c0013[$c0015Darkthorn$c0013] tells you"
                         " 'You need some more experience before I can help "
                         "ya, matey.'\n\r", ch);
            return (TRUE);
        }

        if (!arg || !*arg) {
            /*
             * practice||practise, without argument
             */
            sprintf(buffer, "You have got %d practice sessions left.\n\r",
                    ch->specials.spells_to_learn);
            sprintf(buf, "You can practice any of the following skills.\n\r"
                         "Bear in mind.. training here costs two practice "
                         "points!\n\r\n\r");
            strcat(buffer, buf);
            PrintSkills(ch, 50, styleskillset, styleskillCount, buffer);
            page_string(ch->desc, buffer, 1);
            return (TRUE);
        }

        if (ch->specials.spells_to_learn <= 1) {
            send_to_char("$c0013[$c0015Darkthorn$c0013] tells you"
                         " 'You don't have enough practice points.'\n\r", ch);
            return (TRUE);
        }

        if( LearnSkill(ch, styleskillset, styleskillCount,  arg, 
                       GetMaxLevel(ch), "Darkthorn", 0 ) ) {
            /*
             * LearnSkill already reduced practises by 1, these cost 2
             */
            ch->specials.spells_to_learn = ch->specials.spells_to_learn - 1;
            return( TRUE );
        }

        send_to_char("$c0013[$c0015Darkthorn$c0013] tells you '"
                     "I do not know of that skill!'\n\r", ch);
        return (TRUE);
    }
    return (FALSE);
}


void PrintSkills(struct char_data *ch, int level,
                 const struct skillset *skills, int skillcount, char *buffer)
{
    int             i;
    char            buf[256];


    if( !ch || !skills ) {
        return;
    }

    /*
     * list by level, so new skills show at top of list
     */
    while (level != 0) {
        for( i = 0; i < skillcount; i++ ) {
            if (level == -1 || skills[i].level == level) {
                sprintf(buf, "[%-2d] %-30s %-15s",
                        skills[i].level, skills[i].name,
                        how_good(ch->skills[skills[i].skillnum].learned));

                if (IsSpecialized(ch->skills[skills[i].skillnum].special)) {
                    strcat(buf, " (special)");
                }
                strcat(buf, " \n\r");
                if (strlen(buf) + strlen(buffer) >
                    (MAX_STRING_LENGTH * 2) - 2) {
                    return;
                }
                strcat(buffer, buf);
                strcat(buffer, "\r");
            }
        }

        if( level == -1 ) {
            level = 0;
        } else {
            level--;
        }
    }
}


int LearnSkill(struct char_data *ch, const struct skillset *skills, 
               int skillcount, char *arg, int level, char *teacher, int charge)
{
    int             i;
    int             percent = 0;

    for(i = 0; i < skillcount; i++) {
        if (is_abbrev(arg, skills[i].name)) {
            if (skills[i].level > level) {
                oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'You're not "
                              "experienced enough to learn this skill.'",
                          teacher);
                return (TRUE);
            }

            if (ch->specials.spells_to_learn <= 0) {
                oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'You don't "
                              "have enough practice points.'\n\r", teacher);
                return (TRUE);
            }

            if (charge && GET_GOLD(ch) < charge) {
                oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'Ah, but you "
                              "do not have enough money to pay.'\n\r", teacher);
                return (TRUE);
            }

            if (!charge &&
                ch->skills[skills[i].skillnum].learned > skills[i].maxlearn ) {
                if( skills[i].maxlearn < 94 ) {
                    oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'You must "
                                  "learn from experience and practice to get"
                                  " any better at that skill.'\n\r", teacher);
                } else {
                    oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'You are a"
                                  " master of this art, I can teach you no "
                                  "more.'\n\r", teacher);
                }
                return (TRUE);
            }

            if (ch->skills[skills[i].skillnum].learned >= 95) {
                oldSendOutput(ch, "$c0013[$c0015%s$c0013] tells you 'I have "
                              "taught you all I can about that skill.'\n\r",
                          teacher);
                return (TRUE);
            }

            oldSendOutput(ch, "You practice %s for a while.\n\r", skills[i].name);
            ch->specials.spells_to_learn--;
            GET_GOLD(ch) -= charge;

            if (!IS_SET(ch->skills[skills[i].skillnum].flags, SKILL_KNOWN)) {
                SET_BIT(ch->skills[skills[i].skillnum].flags, SKILL_KNOWN);
            }

            percent = ch->skills[skills[i].skillnum].learned +
                      int_app[(int)GET_INT(ch)].learn;

            ch->skills[skills[i].skillnum].learned = MIN(95, percent);
            if (ch->skills[skills[i].skillnum].learned >= 95) {
                send_to_char("'You are now a master of this art.'\n\r", ch);
            }
            return (TRUE);
        }
    }
    return(FALSE);
}


/*
 * vim:ts=4:sw=4:ai:et:si:sts=4
 */